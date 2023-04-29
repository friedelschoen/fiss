#include "service.h"
#include "util.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


service_t* service_register(const char* name, bool is_log_service) {
	service_t* s;

	if ((s = service_get(name)) == NULL) {
		s				  = &services[services_size++];
		s->state		  = STATE_INACTIVE;
		s->status_change  = time(NULL);
		s->restart_manual = S_DOWN;
		s->restart_file	  = S_DOWN;
		s->last_exit	  = EXIT_NONE;
		s->return_code	  = 0;
		s->fail_count	  = 0;
		s->log_service	  = NULL;
		s->paused		  = false;
		s->log_pipe.read  = 0;
		s->log_pipe.write = 0;
		s->is_log_service = is_log_service;

		strcpy(s->name, name);
	}

	char path_buffer[PATH_MAX];

	snprintf(path_buffer, PATH_MAX, "%s/%s/%s", service_dir, s->name, "log");

	if (s->is_log_service) {
		if (s->log_pipe.read == 0 || s->log_pipe.write == 0)
			pipe((int*) &s->log_pipe);

	} else if (!s->log_service && S_ISDIR(stat_mode("%s/%s/log", service_dir, s->name))) {
		snprintf(path_buffer, PATH_MAX, "%s/%s", s->name, "log");

		if (!s->log_service)
			s->log_service = service_register(path_buffer, true);
	}

	bool autostart, autostart_once;

	autostart	   = S_ISREG(stat_mode("%s/%s/up-%s", service_dir, s->name, runlevel));
	autostart_once = S_ISREG(stat_mode("%s/%s/once-%s", service_dir, s->name, runlevel));

	s->restart_file = S_DOWN;

	if (autostart && autostart_once) {
		fprintf(stderr, "error: %s is marked for up AND once!\n", s->name);
	} else if (autostart) {
		s->restart_file = S_RESTART;
	} else if (autostart_once) {
		if (s->restart_file == S_DOWN)
			s->restart_file = S_ONCE;
	}

	s->status_change = time(NULL);

	return s;
}
