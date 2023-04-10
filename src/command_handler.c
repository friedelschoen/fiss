#include "service.h"

#include <signal.h>
#include <stdio.h>


int service_handle_command(service_t* s, sv_command_t command, unsigned char extra, service_t** response) {
	bool changed = false;
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
			if (extra > 2) {
				return -EBEXT;
			}
			if (extra == 1 || extra == 2) {    // pin
				changed           = !s->restart_manual;
				s->restart_manual = S_RESTART;
			} else {
				s->restart_manual = S_ONCE;
			}
			if (extra == 0 || extra == 1)
				service_start(s, &changed);

			if (!changed)
				return 0;
			response[0] = s;
			return 1;

		case S_STOP:
			if (s == NULL)
				return -ENOSV;
			if (extra > 2) {
				return -EBEXT;
			}
			if (extra == 1 || extra == 2) {    // pin
				changed           = s->restart_manual;
				s->restart_manual = S_NONE;
			}
			if (extra == 0 || extra == 1)
				service_stop(s, &changed);

			if (!changed)
				return 0;
			response[0] = s;
			return 1;

		case S_SEND:
			if (s == NULL)
				return -ENOSV;

			service_send(s, extra);
			return 1;

		case S_PAUSE:
			if (s == NULL)
				return -ENOSV;

			s->paused = true;
			service_send(s, SIGSTOP);
			response[0] = s;
			return 1;

		case S_RESUME:
			if (s == NULL)
				return -ENOSV;

			s->paused = false;
			service_send(s, SIGCONT);
			response[0] = s;
			return 1;

		case S_REVIVE:
			if (s == NULL)
				return -ENOSV;

			s->state = STATE_INACTIVE;
			service_start(s, &changed);

			if (!changed)
				return 0;

			response[0] = s;
			return 1;

		case S_EXIT:
			daemon_running = false;
			return 0;

		default:
			fprintf(stderr, "warning: handling command: unknown command 0x%2x%2x\n", command, extra);
			return -EBADSV;
	}
}
