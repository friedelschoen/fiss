#include "service.h"

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>


service_t   services[SV_SERVICE_MAX];
int         services_size = 0;
char        runlevel[SV_NAME_MAX];
int         service_dir;
const char* service_dir_path;
int         control_socket;
int         null_fd;
service_t*  depends[SV_DEPENDENCY_MAX][2];
int         depends_size;
bool        daemon_running;

service_t* service_get(const char* name) {
	for (int i = 0; i < services_size; i++) {
		if (streq(services[i].name, name))
			return &services[i];
	}
	return NULL;
}

int service_refresh_directory(void) {
	DIR*           dp;
	struct dirent* ep;
	struct stat    st;
	service_t*     s;

	if ((dp = opendir(service_dir_path)) == NULL) {
		print_error("error: cannot open service directory: %s\n");
		return -1;
	}

	for (int i = 0; i < services_size; i++) {
		s = &services[i];
		if (fstat(s->dir, &st) == -1 || !S_ISDIR(st.st_mode)) {
			service_stop(s);
			close(s->dir);
			close(s->control);
		}
	}

	while ((ep = readdir(dp)) != NULL) {
		if (ep->d_name[0] == '.')
			continue;

		if (fstatat(service_dir, ep->d_name, &st, 0) == -1 || !S_ISDIR(st.st_mode))
			continue;

		service_register(service_dir, ep->d_name, false);
	}

	closedir(dp);

	depends_size = 0;
	for (int i = 0; i < services_size; i++)
		service_update_dependency(&services[i]);

	return 0;
}


bool service_need_restart(service_t* s) {
	service_t* d;

	if (!daemon_running)
		return false;

	if (s->restart == S_RESTART)
		return true;

	for (int i = 0; i < depends_size; i++) {
		if (depends[i][1] != s)
			continue;

		d = depends[i][0];
		if (service_need_restart(d))
			return true;
	}

	return false;
}
