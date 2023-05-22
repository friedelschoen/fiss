#include "config.h"
#include "service.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


service_t* service_register(int dir, const char* name, bool is_log_service) {
	service_t* s;

	if ((s = service_get(name)) == NULL) {
		s                 = &services[services_size++];
		s->state          = STATE_INACTIVE;
		s->restart_manual = S_DOWN;
		s->restart_file   = S_DOWN;
		s->last_exit      = EXIT_NONE;
		s->return_code    = 0;
		s->fail_count     = 0;
		s->log_service    = NULL;
		s->paused         = false;
		s->log_pipe.read  = 0;
		s->log_pipe.write = 0;
		s->is_log_service = is_log_service;

		if ((s->dir = openat(dir, name, O_DIRECTORY)) == -1) {
			print_error("error: cannot open '%s': %s\n", name);
			return NULL;
		}

		strncpy(s->name, name, sizeof(s->name));

		service_init_runit(s);

		s->status_change = time(NULL);
		service_update_status(s);
	}

	struct stat st;

	if (s->is_log_service) {
		if (s->log_pipe.read == 0 || s->log_pipe.write == 0)
			pipe((int*) &s->log_pipe);

	} else if (!s->log_service && fstatat(s->dir, "log", &st, 0) != -1 && S_ISDIR(st.st_mode)) {

		if (!s->log_service)
			s->log_service = service_register(s->dir, "log", true);
	}

	bool autostart, autostart_once;

	char up_path[SV_NAME_MAX]   = "up-";
	char once_path[SV_NAME_MAX] = "once-";

	strcat(up_path, runlevel);
	strcat(once_path, runlevel);

	autostart      = fstatat(s->dir, up_path, &st, 0) != -1 && S_ISREG(st.st_mode);
	autostart_once = fstatat(s->dir, once_path, &st, 0) != -1 && S_ISREG(st.st_mode);

	s->restart_file = S_DOWN;

	if (autostart && autostart_once) {
		fprintf(stderr, "error: %s is marked for up AND once!\n", s->name);
	} else if (autostart) {
		s->restart_file = S_RESTART;
	} else if (autostart_once) {
		if (s->restart_file == S_DOWN)
			s->restart_file = S_ONCE;
	}

	return s;
}
