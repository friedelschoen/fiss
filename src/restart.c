#include "config_parser.h"
#include "service.h"
#include "util.h"

#include <errno.h>
#include <limits.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>


static void do_finish(service_t* s) {
	char        path_buffer[PATH_MAX];
	struct stat stat_buffer;
	snprintf(path_buffer, PATH_MAX, "%s/%s/finish", service_dir, s->name);

	if (stat(path_buffer, &stat_buffer) == 0 && stat_buffer.st_mode & S_IEXEC) {
		s->state = STATE_FINISHING;
		if ((s->pid = fork()) == -1) {
			print_error("cannot fork process");
		} else if (s->pid == 0) {
			dup2(null_fd, STDIN_FILENO);
			dup2(null_fd, STDOUT_FILENO);
			dup2(null_fd, STDERR_FILENO);

			execlp(path_buffer, path_buffer, NULL);
			print_error("cannot execute finish process");
			_exit(1);
		}
	} else if (s->fail_count > SV_FAIL_MAX) {
		s->state = STATE_DEAD;
		printf(":: %s died\n", s->name);
	} else {
		s->state = STATE_INACTIVE;
	}
}


void service_check_state(service_t* s, bool signaled, int return_code) {
	s->status_change = time(NULL);
	s->pid           = 0;
	if (s->restart_file == S_ONCE)
		s->restart_file = S_NONE;
	if (s->restart_manual == S_ONCE)
		s->restart_manual = S_NONE;

	char        path_buffer[PATH_MAX];
	struct stat stat_buffer;

	switch (s->state) {
		case STATE_ACTIVE_FOREGROUND:
			if (signaled) {
				s->last_exit   = EXIT_SIGNALED;
				s->return_code = return_code;
				s->fail_count++;

				printf(":: %s killed thought signal %d\n", s->name, s->return_code);
			} else {
				s->last_exit   = EXIT_NORMAL;
				s->return_code = return_code;
				if (s->return_code > 0)
					s->fail_count++;
				else
					s->fail_count = 0;

				printf(":: %s exited with code %d\n", s->name, s->return_code);
			}

			do_finish(s);

			break;
		case STATE_ACTIVE_DUMMY:
		case STATE_ACTIVE_PID:
		case STATE_ACTIVE_BACKGROUND:
		case STATE_STOPPING:
			do_finish(s);
			break;

		case STATE_FINISHING:
			if (s->fail_count > SV_FAIL_MAX) {
				s->state = STATE_DEAD;
				printf(":: %s died\n", s->name);
			} else {
				s->state = STATE_INACTIVE;
			}
			break;
		case STATE_STARTING:
			if (!signaled && return_code == 0) {
				if (snprintf(path_buffer, PATH_MAX, "%s/%s/stop", service_dir, s->name) && stat(path_buffer, &stat_buffer) == 0 && stat_buffer.st_mode & S_IXUSR) {
					s->state = STATE_ACTIVE_BACKGROUND;
				} else if (snprintf(path_buffer, PATH_MAX, "%s/%s/pid", service_dir, s->name) && stat(path_buffer, &stat_buffer) == 0 && stat_buffer.st_mode & S_IRUSR) {
					s->pid   = parse_pid_file(s);
					s->state = STATE_ACTIVE_PID;
				} else {
					do_finish(s);
				}
			} else if (!signaled) {
				s->last_exit   = EXIT_NORMAL;
				s->return_code = return_code;

				do_finish(s);
			} else {    // signaled
				s->last_exit   = EXIT_SIGNALED;
				s->return_code = return_code;

				do_finish(s);
			}
			break;

		case STATE_DEAD:
		case STATE_INACTIVE:
			printf("warn: %s died but was set to inactive\n", s->name);
	}
}
