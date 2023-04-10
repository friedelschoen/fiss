#include "config.h"
#include "service.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

service_t* service_register(string name, bool log_service, bool* changed_ptr) {
	service_t* s;

	if ((s = service_get(name)) != NULL)
		return s;

	s				  = &services[services_size++];
	s->state		  = STATE_INACTIVE;
	s->status_change  = time(NULL);
	s->restart		  = false;
	s->restart_once	  = false;
	s->last_exit	  = EXIT_NONE;
	s->return_code	  = 0;
	s->fail_count	  = 0;
	s->log_service	  = NULL;
	s->paused		  = false;
	s->log_pipe.read  = 0;
	s->log_pipe.write = 0;

	strcpy(s->name, name);

	s->is_log_service = log_service;

	struct stat stat_buf;

	char path_buffer[PATH_MAX];
	snprintf(path_buffer, PATH_MAX, "%s/%s/%s", service_dir, s->name, "log");

	if (s->is_log_service) {
		if (s->log_pipe.read == 0 || s->log_pipe.write == 0)
			pipe((int*) &s->log_pipe);

	} else if (stat(path_buffer, &stat_buf) > -1 && S_ISDIR(stat_buf.st_mode)) {
		snprintf(path_buffer, PATH_MAX, "%s/%s", s->name, "log");

		if (!s->log_service)
			s->log_service = service_register(path_buffer, true, NULL);
	}

	bool autostart, autostart_once;

	snprintf(path_buffer, PATH_MAX, "%s/%s/up-%s", service_dir, s->name, runlevel);
	autostart = stat(path_buffer, &stat_buf) != -1 && S_ISREG(stat_buf.st_mode);

	snprintf(path_buffer, PATH_MAX, "%s/%s/once-%s", service_dir, s->name, runlevel);
	autostart_once = stat(path_buffer, &stat_buf) != -1 && S_ISREG(stat_buf.st_mode);

	if (autostart && autostart_once) {
		fprintf(stderr, "error: %s is marked for up AND once!\n", s->name);
	} else {
		s->restart		= autostart;
		s->restart_once = autostart_once;
	}

	s->status_change = time(NULL);

	if (changed_ptr != NULL)
		*changed_ptr = true;

	return s;
}
