#include "service.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>


static int runit_signals[] = {
	[X_ALARM] = SIGALRM,
	[X_HUP]   = SIGHUP,
	[X_INT]   = SIGINT,
	[X_QUIT]  = SIGQUIT,
	[X_USR1]  = SIGUSR1,
	[X_USR2]  = SIGUSR2,
};

void service_update_status(service_t* s) {
	int fd;
	if ((fd = openat(s->dir, "supervise/status", O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1) {
		print_error("cannot open supervise/status: %s\n");
		return;
	}

	service_serial_t stat_runit;
	service_encode(s, &stat_runit);

	if (write(fd, &stat_runit, sizeof(stat_runit)) == -1) {
		print_error("cannot write to supervise/status: %s\n");
		return;
	}

	close(fd);

	if ((fd = openat(s->dir, "supervise/stat", O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1) {
		print_error("cannot create supervise/stat: %s\n");
		return;
	}

	const char* stat_human = service_status_name(s);
	if (write(fd, stat_human, strlen(stat_human)) == -1) {
		print_error("cannot write to supervise/stat: %s\n");
		return;
	}

	close(fd);

	if ((fd = openat(s->dir, "supervise/pid", O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1) {
		print_error("cannot create supervise/stat: %s\n");
		return;
	}

	dprintf(fd, "%d", s->pid);

	close(fd);
}

#if 0
void service_handle_command_runit(service_t* s, sv_command_runit_t command) {
#	if SV_RUNIT_COMPAT != 0
	switch (command) {
		case R_DOWN:
		case R_TERM:
			s->restart_manual = S_FORCE_DOWN;
			service_stop(s, NULL);
			break;
		case R_UP:
			s->restart_manual = S_RESTART;
			service_start(s, NULL);
			break;
		case R_ONCE:
			s->restart_manual = S_ONCE;
			service_start(s, NULL);
			break;
		case R_KILL:
			s->restart_manual = S_FORCE_DOWN;
			service_kill(s, SIGKILL);
			break;
		case R_PAUSE:
			if (!s->paused) {
				s->paused = true;
				service_kill(s, SIGSTOP);
			}
			break;
		case R_CONT:
			if (s->paused) {
				s->paused = false;
				service_kill(s, SIGCONT);
			}
			break;
		case R_ALARM:
		case R_HUP:
		case R_INT:
		case R_QUIT:
		case R_USR1:
		case R_USR2:
			service_kill(s, runit_signals[command]);
			break;
		case R_EXIT:
			// ignored
			return;
	}

	s->status_change = time(NULL);
	service_update_status(s);
#	endif
}
#endif
