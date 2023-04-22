#include "service.h"

#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>


int service_handle_command(void* argv, sv_command_t command, unsigned char extra, service_t** response) {
	service_t* s       = argv;
	bool       changed = false;
	char       path_buffer[PATH_MAX];
	int        fd;

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
			response[0] = s;
			return 1;

		case S_SEND:
			if (s == NULL)
				return -ENOSV;

			service_send(s, extra);
			response[0] = s;
			return 1;

		case S_PAUSE:
			if (s == NULL)
				return -ENOSV;
			if (s->state == STATE_INACTIVE || s->paused)
				return 0;

			s->paused = true;
			service_send(s, SIGSTOP);

			response[0] = s;
			return 1;

		case S_RESUME:
			if (s == NULL)
				return -ENOSV;
			if (s->state == STATE_INACTIVE || !s->paused)
				return 0;

			s->paused = false;
			service_send(s, SIGCONT);

			response[0] = s;
			return 1;

		case S_REVIVE:
			if (s == NULL)
				return -ENOSV;

			if (s->state != STATE_DEAD)
				return 0;

			s->state = STATE_INACTIVE;

			response[0] = s;
			return 1;

		case S_EXIT:
			daemon_running = false;
			return 0;

		case S_SWITCH:
			if (argv == NULL)
				return -ENOSV;

			strcpy(runlevel, argv);

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

			if (extra == 1)    // once
				snprintf(path_buffer, PATH_MAX, "%s/%s/once-%s", service_dir, s->name, runlevel);
			else
				snprintf(path_buffer, PATH_MAX, "%s/%s/up-%s", service_dir, s->name, runlevel);

			if (command == S_ENABLE) {
				if ((fd = open(path_buffer, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1)
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
