#include "config_parser.h"
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
		s->state = STATE_FINISHING;
		if ((s->pid = fork_dup_cd_exec(s->dir, "./finish", null_fd, null_fd, null_fd)) == -1) {
			print_error("error: cannot execute ./finish: %s\n");
			s->state = STATE_INACTIVE;
		}
	} else if (s->fail_count == SV_FAIL_MAX) {
		s->state = STATE_DEAD;
		printf("%s died\n", s->name);
	} else {
		s->state = STATE_INACTIVE;
	}
}


void service_check_state(service_t* s, bool signaled, int return_code) {
	s->status_change = time(NULL);
	s->pid           = 0;
	if (s->restart_file == S_ONCE)
		s->restart_file = S_DOWN;
	if (s->restart_manual == S_ONCE)
		s->restart_manual = S_DOWN;

	struct stat st;

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
		case STATE_ACTIVE_PID:
		case STATE_ACTIVE_BACKGROUND:
		case STATE_STOPPING:
			do_finish(s);
			break;

		case STATE_FINISHING:
			if (s->fail_count == SV_FAIL_MAX) {
				s->state = STATE_DEAD;
				printf("%s died\n", s->name);
			} else {
				s->state = STATE_INACTIVE;
			}
			break;
		case STATE_STARTING:
			if (!signaled && return_code == 0) {
				if (fstatat(s->dir, "stop", &st, 0) != -1 && st.st_mode & S_IXUSR) {
					s->state = STATE_ACTIVE_BACKGROUND;
				} else if (fstatat(s->dir, "pid", &st, 0) != -1 && st.st_mode & S_IXUSR) {
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
