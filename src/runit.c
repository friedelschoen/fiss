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
	[R_ALARM] = SIGALRM,
	[R_HUP]   = SIGHUP,
	[R_INT]   = SIGINT,
	[R_QUIT]  = SIGQUIT,
	[R_USR1]  = SIGUSR1,
	[R_USR2]  = SIGUSR2,
};

void service_init_status(service_t* s) {
#if SV_RUNIT_COMPAT != 0
	int         lockfd;
	struct stat st;

	if (fstatat(s->dir, "supervise", &st, 0) == -1)
		mkdirat(s->dir, "supervise", 0744);

	if (fstatat(s->dir, "supervise/ok", &st, 0) == -1)
		mkfifoat(s->dir, "supervise/ok", 0644);

	if (fstatat(s->dir, "supervise/control", &st, 0) == -1)
		mkfifoat(s->dir, "supervise/control", 0644);

	if (openat(s->dir, "supervise/ok", O_RDONLY | O_NONBLOCK) == -1) {
		print_error("cannot open supervise/ok: %s\n");
		return;
	}

	if ((s->control = openat(s->dir, "supervise/control", O_RDONLY | O_NONBLOCK)) == -1) {
		print_error("cannot open supervise/ok: %s\n");
		return;
	}

	if ((lockfd = openat(s->dir, "supervise/lock", O_CREAT | O_WRONLY, 0644)) == -1) {
		print_error("cannot create supervise/lock: %s\n");
		return;
	}
	close(lockfd);
#endif
}

void service_update_status(service_t* s) {
#if SV_RUNIT_COMPAT != 0
	int fd;
	if ((fd = openat(s->dir, "supervise/status", O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1) {
		print_error("cannot open supervise/status: %s\n");
		return;
	}

	uint8_t stat_runit[SV_SERIAL_RUNIT_LEN];
	service_store_runit(s, stat_runit);

	if (write(fd, stat_runit, sizeof(stat_runit)) == -1) {
		print_error("cannot write to supervise/status: %s\n");
		return;
	}

	close(fd);

	if ((fd = openat(s->dir, "supervise/stat", O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1) {
		print_error("cannot create supervise/stat: %s\n");
		return;
	}

	const char* stat_human = service_store_human(s);
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
#endif
}

void service_handle_command_runit(service_t* s, sv_command_runit_t command) {
#if SV_RUNIT_COMPAT != 0
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
			service_send(s, SIGKILL);
			break;
		case R_PAUSE:
			if (!s->paused) {
				s->paused = true;
				service_send(s, SIGSTOP);
			}
			break;
		case R_CONT:
			if (s->paused) {
				s->paused = false;
				service_send(s, SIGCONT);
			}
			break;
		case R_ALARM:
		case R_HUP:
		case R_INT:
		case R_QUIT:
		case R_USR1:
		case R_USR2:
			service_send(s, runit_signals[command]);
			break;
		case R_EXIT:
			// ignored
			return;
	}

	s->status_change = time(NULL);
	service_update_status(s);
#endif
}
