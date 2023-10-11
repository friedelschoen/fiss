#include "service.h"

#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


static int runit_signals[] = {
	[X_ALARM] = SIGALRM,
	[X_HUP]   = SIGHUP,
	[X_INT]   = SIGINT,
	[X_QUIT]  = SIGQUIT,
	[X_USR1]  = SIGUSR1,
	[X_USR2]  = SIGUSR2,
};

void service_handle_command(struct service* s, char command) {
	switch ((enum service_command) command) {
		case X_UP:
			s->restart = S_RESTART;
			service_start(s);
			break;
		case X_ONCE:
			s->restart = S_ONCE;
			service_start(s);
			break;
		case X_DOWN:
		case X_TERM:
			s->restart = S_DOWN;
			service_stop(s);
			break;
		case X_KILL:
			s->restart = S_DOWN;
			service_kill(s, SIGKILL);
			break;
		case X_PAUSE:
			if (!s->paused) {
				s->paused = true;
				service_kill(s, SIGSTOP);
			}
			break;
		case X_CONT:
			if (s->paused) {
				s->paused = false;
				service_kill(s, SIGCONT);
			}
			break;
		case X_ALARM:
		case X_HUP:
		case X_INT:
		case X_QUIT:
		case X_USR1:
		case X_USR2:
			service_kill(s, runit_signals[(int) command]);
			break;
		case X_RESET:
			if (s->paused) {
				s->paused = false;
				service_kill(s, SIGCONT);
			}

			s->fail_count = 0;
			if (s->state == STATE_ERROR)
				service_update_state(s, STATE_INACTIVE);

			break;
		case X_EXIT:
			// ignored
			return;
	}
}
