#include "service.h"
#include "util.h"

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

			s->status_change = time(NULL);
			service_update_status(s);
			break;
		case STATE_ACTIVE_FOREGROUND:
		case STATE_ACTIVE_PID:
		case STATE_SETUP:
			kill(s->pid, SIGTERM);
			if (changed)
				*changed = true;

			s->status_change = time(NULL);
			service_update_status(s);
			break;
		case STATE_ACTIVE_BACKGROUND:
			s->state = STATE_STOPPING;
			if ((s->pid = fork_dup_cd_exec(s->dir, "./stop", null_fd, null_fd, null_fd)) == -1) {
				print_error("error: cannot execute ./stop: %s\n");
				s->state = STATE_INACTIVE;
			}
			if (changed)
				*changed = true;

			s->status_change = time(NULL);
			service_update_status(s);
			break;
		case STATE_STARTING:
		case STATE_STOPPING:
		case STATE_FINISHING:
			kill(s->pid, SIGTERM);
			if (changed)
				*changed = true;

			s->status_change = time(NULL);
			service_update_status(s);
			break;
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
