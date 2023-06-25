#include "parse.h"
#include "service.h"

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


static void do_finish(service_t* s) {
	struct stat st;

	if (fstatat(s->dir, "finish", &st, 0) != -1 && st.st_mode & S_IXUSR) {
		if ((s->pid = fork_dup_cd_exec(s->dir, "./finish", null_fd, null_fd, null_fd)) == -1) {
			print_error("error: cannot execute ./finish: %s\n");
			service_update_state(s, STATE_INACTIVE);
		} else {
			service_update_state(s, STATE_FINISHING);
		}
	} else if (s->fail_count == SV_FAIL_MAX) {
		service_update_state(s, STATE_ERROR);
		printf("%s died\n", s->name);
	} else {
		service_update_state(s, STATE_INACTIVE);
	}
}


void service_handle_exit(service_t* s, bool signaled, int return_code) {
	struct stat st;

	s->pid          = 0;
	s->stop_timeout = 0;

	if (s->restart == S_ONCE)
		s->restart = S_DOWN;

	switch (s->state) {
		case STATE_SETUP:
			service_run(s);
			break;
		case STATE_ACTIVE_FOREGROUND:
			if (signaled) {
				s->last_exit   = EXIT_SIGNALED;
				s->return_code = return_code;
				s->fail_count++;

				printf("%s killed thought signal %d\n", s->name, s->return_code);
			} else {
				s->last_exit   = EXIT_NORMAL;
				s->return_code = return_code;
				if (s->return_code > 0)
					s->fail_count++;
				else
					s->fail_count = 0;

				printf("%s exited with code %d\n", s->name, s->return_code);
			}

			do_finish(s);

			break;
		case STATE_ACTIVE_DUMMY:
		case STATE_ACTIVE_BACKGROUND:
		case STATE_STOPPING:
			do_finish(s);
			break;

		case STATE_FINISHING:
			if (s->fail_count == SV_FAIL_MAX) {
				service_update_state(s, STATE_ERROR);
				printf("%s died\n", s->name);
			} else {
				service_update_state(s, STATE_INACTIVE);
			}
			break;
		case STATE_STARTING:
			if (!signaled && return_code == 0) {
				if (fstatat(s->dir, "stop", &st, 0) != -1 && st.st_mode & S_IXUSR) {
					service_update_state(s, STATE_ACTIVE_BACKGROUND);
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

		case STATE_ERROR:
		case STATE_INACTIVE:
			printf("warn: %s died but was set to inactive\n", s->name);
	}
}
