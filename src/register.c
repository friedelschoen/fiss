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


static int init_supervise(struct service* s) {
	int         fd;
	struct stat st;

	if (fstatat(s->dir, "supervise", &st, 0) == -1 && mkdirat(s->dir, "supervise", 0755) == -1) {
		return -1;
	}

	if (fstatat(s->dir, "supervise/ok", &st, 0) == -1 && mkfifoat(s->dir, "supervise/ok", 0666) == -1) {
		print_errno("cannot create fifo at supervise/ok: %s\n");
		return -1;
	}

	if (fstatat(s->dir, "supervise/control", &st, 0) == -1 && mkfifoat(s->dir, "supervise/control", 0644) == -1) {
		print_errno("cannot create fifo at supervise/control: %s\n");
		return -1;
	}

	if (openat(s->dir, "supervise/ok", O_RDONLY | O_NONBLOCK) == -1) {
		print_errno("cannot open supervise/ok: %s\n");
		return -1;
	}

	if ((s->control = openat(s->dir, "supervise/control", O_RDONLY | O_NONBLOCK)) == -1) {
		print_errno("cannot open supervise/ok: %s\n");
		return -1;
	}

	if ((fd = openat(s->dir, "supervise/lock", O_CREAT | O_WRONLY, 0644)) == -1) {
		print_errno("cannot create supervise/lock: %s\n");
		return -1;
	}
	close(fd);

	return 0;
}

struct service* service_register(int dir, const char* name, bool is_log_service) {
	struct service* s;
	struct stat     st;

	if ((s = service_get(name)) == NULL) {
		s                 = &services[services_size++];
		s->state          = STATE_INACTIVE;
		s->restart        = S_DOWN;
		s->last_exit      = EXIT_NONE;
		s->return_code    = 0;
		s->fail_count     = 0;
		s->log_service    = NULL;
		s->paused         = false;
		s->log_pipe.read  = 0;
		s->log_pipe.write = 0;
		s->is_log_service = is_log_service;
		s->stop_timeout   = 0;

		if ((s->dir = openat(dir, name, O_DIRECTORY)) == -1) {
			print_errno("error: cannot open '%s': %s\n", name);
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

	service_write(s);

	return s;
}
