#include "service.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>


void service_update_state(service_t* s, int state) {
	if (state != -1)
		s->state = state;

	s->state_change = time(NULL);

	service_write(s);
}

void service_write(service_t* s) {
	int                   fd;
	const char*           stat_human;
	struct service_serial stat_runit;

	if ((fd = openat(s->dir, "supervise/status.new", O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1) {
		print_error("cannot open supervise/status: %s\n");
		return;
	}

	service_encode(s, &stat_runit);

	if (write(fd, &stat_runit, sizeof(stat_runit)) == -1) {
		print_error("cannot write to supervise/status: %s\n");
		return;
	}

	close(fd);

	if ((fd = openat(s->dir, "supervise/stat.new", O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1) {
		print_error("cannot create supervise/stat: %s\n");
		return;
	}

	stat_human = service_status_name(s);
	if (write(fd, stat_human, strlen(stat_human)) == -1) {
		print_error("cannot write to supervise/stat: %s\n");
		return;
	}

	close(fd);

	if ((fd = openat(s->dir, "supervise/pid.new", O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1) {
		print_error("cannot create supervise/stat: %s\n");
		return;
	}

	dprintf(fd, "%d", s->pid);

	close(fd);

	renameat(s->dir, "supervise/status.new", s->dir, "supervise/status");
	renameat(s->dir, "supervise/stat.new", s->dir, "supervise/stat");
	renameat(s->dir, "supervise/pid.new", s->dir, "supervise/pid");
}

const char* service_status_name(service_t* s) {
	switch (s->state) {
		case STATE_SETUP:
			return "setup";
		case STATE_STARTING:
			return "starting";
		case STATE_ACTIVE_FOREGROUND:
			return "run";
		case STATE_ACTIVE_BACKGROUND:
			return "run-background";
		case STATE_ACTIVE_DUMMY:
			return "run-dummy";
		case STATE_FINISHING:
			return "finishing";
		case STATE_STOPPING:
			return "stopping";
		case STATE_INACTIVE:
			return "down";
		case STATE_ERROR:
			return "dead (error)";
		default:
			return NULL;
	}
}
