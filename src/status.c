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


void service_update_status(service_t* s) {
	int              fd;
	const char*      stat_human;
	service_serial_t stat_runit;

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
