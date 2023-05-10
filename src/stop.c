#include "service.h"

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


void service_stop(service_t* s, bool* changed) {
	switch (s->state) {
		case STATE_ACTIVE_DUMMY:
			service_check_state(s, false, 0);
			if (changed)
				*changed = true;
			break;
		case STATE_ACTIVE_FOREGROUND:
		case STATE_ACTIVE_PID:
		case STATE_SETUP:
			kill(s->pid, SIGTERM);
			if (changed)
				*changed = true;
			break;
		case STATE_ACTIVE_BACKGROUND:
			s->state = STATE_STOPPING;
			if ((s->pid = fork()) == -1) {
				print_error("error: cannot fork process: %s\n");
			} else if (s->pid == 0) {
				dup2(null_fd, STDIN_FILENO);
				dup2(null_fd, STDOUT_FILENO);
				dup2(null_fd, STDERR_FILENO);

				fchdir(s->dir);
				execl("./stop", "./stop", NULL);
				print_error("error: cannot execute stop process: %s\n");
				_exit(1);
			}
			if (changed)
				*changed = true;
			break;
		case STATE_STARTING:
		case STATE_STOPPING:
		case STATE_FINISHING:
			kill(s->pid, SIGTERM);
			if (changed)
				*changed = true;

		case STATE_INACTIVE:
		case STATE_DEAD:
			break;
	}
}

void service_send(service_t* s, int signal) {
	if (!s->pid)
		return;

	if (s->state == STATE_ACTIVE_FOREGROUND || s->state == STATE_ACTIVE_PID)
		kill(s->pid, signal);
}
