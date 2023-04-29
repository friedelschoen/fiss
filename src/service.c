#include "service.h"

#include "pattern.h"

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>


service_t	 services[SV_SERVICE_MAX];
int			 services_size = 0;
char		 runlevel[SV_NAME_MAX];
const char*	 service_dir;
int			 control_socket;
int			 null_fd;
bool		 verbose = false;
dependency_t depends[SV_DEPENDENCY_MAX];
int			 depends_size;


service_t* service_get(const char* name) {
	for (int i = 0; i < services_size; i++) {
		if (streq(services[i].name, name))
			return &services[i];
	}
	return NULL;
}

int service_pattern(const char* name, service_t** dest, int dest_max) {
	int size = 0;
	for (int i = 0; i < services_size && size < dest_max; i++) {
		if (pattern_test(name, services[i].name))
			dest[size++] = &services[i];
	}
	return size;
}

int service_refresh() {
	DIR*		   dp;
	struct dirent* ep;
	dp = opendir(service_dir);
	if (dp == NULL) {
		print_error("cannot open directory %s", service_dir);
		return -1;
	}

	struct stat stat_str;
	char		path_buffer[PATH_MAX];

	for (int i = 0; i < services_size; i++) {
		service_t* s = &services[i];
		snprintf(path_buffer, PATH_MAX, "%s/%s", service_dir, s->name);
		if (stat(path_buffer, &stat_str) == -1 || !S_ISDIR(stat_str.st_mode)) {
			if (s->pid)
				kill(s->pid, SIGKILL);
			if (i < services_size - 1) {
				memmove(services + i, services + i + 1, services_size - i - 1);
				i--;
			}
			services_size--;
		}
	}

	while ((ep = readdir(dp)) != NULL) {
		if (ep->d_name[0] == '.')
			continue;
		snprintf(path_buffer, PATH_MAX, "%s/%s", service_dir, ep->d_name);
		if (stat(path_buffer, &stat_str) == -1 || !S_ISDIR(stat_str.st_mode))
			continue;

		service_register(ep->d_name, false);
	}

	closedir(dp);

	depends_size = 0;
	for (int i = 0; i < services_size; i++)
		service_update_dependency(&services[i]);

	return 0;
}


static bool is_dependency(service_t* d) {
	service_t* s;
	for (int i = 0; i < depends_size; i++) {
		s = depends[i].service;
		if (depends[i].depends == d && (s->state != STATE_INACTIVE || service_need_restart(s)))
			return true;
	}
	return false;
}

bool service_need_restart(service_t* s) {
	if (s->restart_manual == S_FORCE_DOWN)
		return is_dependency(s);
	return s->restart_file == S_ONCE ||
		   s->restart_file == S_RESTART ||
		   s->restart_manual == S_ONCE ||
		   s->restart_manual == S_RESTART ||
		   is_dependency(s);
}