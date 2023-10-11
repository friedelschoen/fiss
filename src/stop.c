#include "service.h"
#include "util.h"

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


void service_stop(struct service* s) {
	switch (s->state) {
		case STATE_ACTIVE_DUMMY:
			service_handle_exit(s, false, 0);
			break;
		case STATE_ACTIVE_BACKGROUND:
			if ((s->pid = fork_dup_cd_exec(s->dir, "./stop", null_fd, null_fd, null_fd)) == -1) {
				print_errno("error: cannot execute ./stop: %s\n");
				service_update_state(s, STATE_INACTIVE);
			} else {
				service_update_state(s, STATE_STOPPING);
			}
			break;
		case STATE_ACTIVE_FOREGROUND:
		case STATE_SETUP:
		case STATE_STARTING:
		case STATE_STOPPING:
		case STATE_FINISHING:
			s->stop_timeout = time(NULL);
			kill(s->pid, SIGTERM);
			service_update_state(s, -1);
			break;
		case STATE_DONE:
			s->state = STATE_INACTIVE;
		case STATE_INACTIVE:
		case STATE_ERROR:
			break;
	}
}

void service_kill(struct service* s, int signal) {
	if (!s->pid)
		return;

	if (s->state == STATE_ACTIVE_FOREGROUND)
		kill(s->pid, signal);
}
