#include "config.h"
#include "service.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


static int fd_set_flag(int fd, int flags) {
	int rc;

	if ((rc = fcntl(fd, F_GETFL)) == -1)
		return -1;

	if (fcntl(fd, F_SETFL, rc | flags) == -1)
		return -1;

	return 0;
}

static int init_supervise(service_t* s) {
	int         fd;
	struct stat st;

	if (fstatat(s->dir, "supervise", &st, 0) == -1 && mkdirat(s->dir, "supervise", 0744) == -1) {
		print_error("warning: cannot create directory supervise: %s\ntrying to remove supervise...\n");
		if (unlinkat(s->dir, "supervise", 0) == -1 || mkdirat(s->dir, "supervise", 0744) == -1) {
			print_error("warning: cannot create directory supervise: %s\n");
			return -1;
		}
	}
	if (fstatat(s->dir, "supervise/ok", &st, 0) == -1 && mkfifoat(s->dir, "supervise/ok", 0644) == -1) {
		print_error("cannot create fifo at supervise/ok: %s\n");
		return -1;
	}
	if (fstatat(s->dir, "supervise/control", &st, 0) == -1 && mkfifoat(s->dir, "supervise/control", 0644) == -1) {
		print_error("cannot create fifo at supervise/control: %s\n");
		return -1;
	}
	if (openat(s->dir, "supervise/ok", O_RDONLY | O_NONBLOCK) == -1) {
		print_error("cannot open supervise/ok: %s\n");
		return -1;
	}

	if ((s->control = openat(s->dir, "supervise/control", O_RDONLY | O_NONBLOCK)) == -1) {
		print_error("cannot open supervise/ok: %s\n");
		return -1;
	}

	if (fd_set_flag(s->control, O_NONBLOCK)) {
		print_error("cannot set supervise/control non-blocking: %s\n");
	}

	if ((fd = openat(s->dir, "supervise/lock", O_CREAT | O_WRONLY, 0644)) == -1) {
		print_error("cannot create supervise/lock: %s\n");
		return -1;
	}
	close(fd);


	if ((fd = openat(s->dir, "supervise/runlevel", O_CREAT | O_TRUNC | O_WRONLY, 0644)) == -1) {
		print_error("cannot create supervise/runlevel: %s\n");
		return -1;
	}

	if (write(fd, runlevel, strlen(runlevel)) == -1) {
		print_error("cannot write to supervise/runlevel: %s\n");
		close(fd);
		return -1;
	}
	close(fd);

	return 0;
}

service_t* service_register(int dir, const char* name, bool is_log_service) {
	service_t*  s;
	struct stat st;

	char up_path[SV_NAME_MAX]   = "up-",
	     once_path[SV_NAME_MAX] = "once-";


	if ((s = service_get(name)) == NULL) {
		s                 = &services[services_size++];
		s->state          = STATE_INACTIVE;
		s->restart_manual = S_DOWN;
		s->restart_file   = S_DOWN;
		s->last_exit      = EXIT_NONE;
		s->return_code    = 0;
		s->fail_count     = 0;
		s->log_service    = NULL;
		s->paused         = false;
		s->log_pipe.read  = 0;
		s->log_pipe.write = 0;
		s->is_log_service = is_log_service;

		if ((s->dir = openat(dir, name, O_DIRECTORY)) == -1) {
			print_error("error: cannot open '%s': %s\n", name);
			services_size--;
			return NULL;
		}

		if (init_supervise(s) == -1) {
			services_size--;
			return NULL;
		}

		strncpy(s->name, name, sizeof(s->name));

		service_update_state(s, -1);
	}

	if (s->is_log_service) {
		if (s->log_pipe.read == 0 || s->log_pipe.write == 0)
			pipe((int*) &s->log_pipe);

	} else if (!s->log_service && fstatat(s->dir, "log", &st, 0) != -1 && S_ISDIR(st.st_mode)) {

		if (!s->log_service)
			s->log_service = service_register(s->dir, "log", true);
	}

	strcat(up_path, runlevel);
	strcat(once_path, runlevel);

	s->restart_file = S_DOWN;

	if (fstatat(s->dir, up_path, &st, 0) != -1 && st.st_mode & S_IREAD)
		s->restart_file = S_RESTART;
	else if (fstatat(s->dir, once_path, &st, 0) != -1 && st.st_mode & S_IREAD)
		s->restart_file = S_ONCE;


	return s;
}
