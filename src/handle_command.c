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

void service_handle_command(service_t* s, service_command_t command, char data) {
	switch (command) {
		case X_UP:
			s->restart_manual = S_RESTART;
			service_start(s);
			break;
		case X_ONCE:
			s->restart_manual = S_ONCE;
			service_start(s);
			break;
		case X_DOWN:
		case X_TERM:
			s->restart_manual = S_FORCE_DOWN;
			service_stop(s);
			break;
		case X_XUP:
			switch (data) {
				case 'd':
					s->restart_manual = S_DOWN;
					break;
				case 'f':
					s->restart_manual = S_FORCE_DOWN;
					break;
				case 'o':
					s->restart_manual = S_ONCE;
					break;
				case 'u':
					s->restart_manual = S_RESTART;
					break;
			}
			service_start(s);
			break;
		case X_XDOWN:
			switch (data) {
				case 'd':
					s->restart_manual = S_DOWN;
					break;
				case 'f':
					s->restart_manual = S_FORCE_DOWN;
					break;
				case 'o':
					s->restart_manual = S_ONCE;
					break;
				case 'u':
					s->restart_manual = S_RESTART;
					break;
			}
			service_stop(s);
			break;
		case X_KILL:
			s->restart_manual = S_FORCE_DOWN;
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
			service_kill(s, runit_signals[command]);
			break;
		case X_RESET:
			if (s->paused) {
				s->paused = false;
				service_kill(s, SIGCONT);
			}

			s->fail_count = 0;
			if (s->state == STATE_DEAD)
				s->state = STATE_INACTIVE;

			s->status_change = time(NULL);
			service_update_status(s);
			break;
		case X_EXIT:
			// ignored
			return;
	}

	s->status_change = time(NULL);
	service_update_status(s);
}
