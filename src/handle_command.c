#include "service.h"

#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


#if 0
int service_handle_command(service_t* s, service_command_t command) {
	bool changed = false;
	char path_buffer[PATH_MAX];
	int  fd;

	switch (command) {
		case S_STATUS:
			if (s != NULL) {
				response[0] = s;
				return 1;
			}
			for (int i = 0; i < services_size; i++) {
				response[i] = &services[i];
			}
			return services_size;

		case S_START:
			if (s == NULL)
				return -ENOSV;
			if (extra > 1) {
				return -EBEXT;
			}
			if (s->state != STATE_INACTIVE)
				return 0;

			if (extra == 1) {    // pin
				changed           = s->restart_manual != S_RESTART;
				s->restart_manual = S_RESTART;
			} else {
				changed           = s->restart_manual != S_ONCE;
				s->restart_manual = S_ONCE;
			}
			if (extra == 0 || extra == 1)
				service_start(s, &changed);

			if (!changed)
				return 0;

			s->status_change = time(NULL);
			service_update_status(s);

			response[0] = s;
			return 1;

		case S_STOP:
			if (s == NULL)
				return -ENOSV;
			if (extra > 1) {
				return -EBEXT;
			}
			if (s->state == STATE_INACTIVE)
				return 0;
			if (extra == 1) {    // pin
				changed           = s->restart_manual != S_FORCE_DOWN;
				s->restart_manual = S_FORCE_DOWN;
			} else {
				changed           = s->restart_manual != S_DOWN;
				s->restart_manual = S_DOWN;
			}
			if (extra == 0 || extra == 1)
				service_stop(s, &changed);

			if (!changed)
				return 0;

			s->status_change = time(NULL);
			service_update_status(s);

			response[0] = s;
			return 1;

		case S_SEND:
			if (s == NULL)
				return -ENOSV;

			service_kill(s, extra);
			response[0] = s;
			return 1;

		case S_PAUSE:
			if (s == NULL)
				return -ENOSV;

			if (s->state == STATE_INACTIVE || s->paused)
				return 0;

			s->paused = true;
			service_kill(s, SIGSTOP);

			s->status_change = time(NULL);
			service_update_status(s);

			response[0] = s;
			return 1;

		case S_RESUME:
			if (s == NULL)
				return -ENOSV;

			if (s->state == STATE_INACTIVE || s->state == STATE_DEAD || s->pid == 0 || !s->paused)
				return 0;

			s->paused = false;
			service_kill(s, SIGCONT);

			s->status_change = time(NULL);
			service_update_status(s);

			response[0] = s;
			return 1;

		case S_RESET:
			if (s == NULL)
				return -ENOSV;

			if (s->paused) {
				s->paused = false;
				service_kill(s, SIGCONT);
			}

			s->fail_count = 0;
			if (s->state == STATE_DEAD)
				s->state = STATE_INACTIVE;

			s->status_change = time(NULL);
			service_update_status(s);

			response[0] = s;
			return 1;

		case S_EXIT:
			daemon_running = false;
			return 0;

		case S_SWITCH:
			if (argv == NULL)
				return -ENOSV;

			strncpy(runlevel, argv, SV_NAME_MAX);

			if (extra == 1) {
				for (int i = 0; i < services_size; i++) {
					services[i].restart_manual = S_DOWN;
				}
			}

			return 0;

		case S_ENABLE:
		case S_DISABLE:
			if (argv == NULL)
				return -ENOSV;

			strcpy(path_buffer, extra == 1 ? "once-" : "up-");
			strcat(path_buffer, runlevel);

			if (command == S_ENABLE) {
				if ((fd = openat(s->dir, path_buffer, O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1)
					return 0;
				close(fd);
			} else {
				if (remove(path_buffer) == -1)
					return 0;
			}

			response[0] = s;
			return 1;
	}
	fprintf(stderr, "warning: handling command: unknown command 0x%2x%2x\n", command, extra);
	return -EBADSV;
}
#endif

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
