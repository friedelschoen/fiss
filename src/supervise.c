#include "config.h"
#include "service.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>


static void signal_child(int unused) {
	(void) unused;

	int        status;
	pid_t      died_pid;
	service_t* s = NULL;

	if ((died_pid = wait(&status)) == -1) {
		print_error("error: cannot wait for process: %s\n");
		return;
	}

	if (!WIFEXITED(status) && !WIFSIGNALED(status))
		return;

	for (int i = 0; i < services_size; i++) {
		if (services[i].pid == died_pid) {
			s = &services[i];
			break;
		}
	}
	if (s == NULL)
		return;

	service_handle_exit(s, WIFSIGNALED(status), WIFSIGNALED(status) ? WTERMSIG(status) : WEXITSTATUS(status));
}

static void update_services(void) {
	service_t* s;

	for (int i = 0; i < services_size; i++) {
		s = &services[i];
		if (s->state == STATE_ERROR)
			continue;

		if (s->stop_timeout != 0) {
			if (s->state == STATE_INACTIVE || s->state == STATE_ERROR) {
				s->stop_timeout = 0;
			} else if (time(NULL) - s->stop_timeout >= SV_STOP_TIMEOUT) {
				printf(":: service '%s' doesn't terminate, killing...\n", s->name);
				service_kill(s, SIGKILL);
				s->stop_timeout = 0;
			}
			continue;
		}

		if (s->state == STATE_INACTIVE && service_need_restart(s)) {
			service_start(s);
		}
	}
}

static void control_sockets(void) {
	service_t* s;
	char       cmd;

	for (int i = 0; i < services_size; i++) {
		s = &services[i];
		while (read(s->control, &cmd, 1) == 1) {
			printf("handling '%c' from %s\n", cmd, s->name);
			service_handle_command(s, cmd);
		}
	}
}

void stop_dummies(void) {
	bool cont;
	for (int i = 0; i < services_size; i++) {
		if (services[i].state != STATE_ACTIVE_DUMMY || services[i].restart == S_RESTART)
			continue;

		cont = false;
		for (int i = 0; i < depends_size; i++) {
			if (depends[i][0] != &services[i])
				continue;

			if (depends[i][1]->state != STATE_INACTIVE || depends[i][1]->state != STATE_ERROR) {
				cont = true;
			}
		}
		if (!cont) {
			service_stop(&services[i]);
		}
	}
}

int service_supervise(const char* service_dir_, const char* service, bool once) {
	struct sigaction sigact = { 0 };
	service_t*       s;

	daemon_running = true;

	sigact.sa_handler = signal_child;
	sigaction(SIGCHLD, &sigact, NULL);
	sigact.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sigact, NULL);

	service_dir_path = service_dir_;
	if ((service_dir = open(service_dir_, O_DIRECTORY)) == -1) {
		print_error("error: cannot open directory %s: %s\n", service_dir_);
		return 1;
	}

	if ((null_fd = open("/dev/null", O_RDWR)) == -1) {
		print_error("error: cannot open /dev/null: %s\n");
		null_fd = 1;
	}

	printf(":: starting services\n");

	service_refresh_directory();

	if ((s = service_get(service)) == NULL) {
		fprintf(stderr, "error: cannot start '%s': not found\n", service);
		goto cleanup;
	}

	s->restart = once ? S_ONCE : S_RESTART;
	service_start(s);


	bool cont;
	// accept connections and handle requests
	do {
		if (!daemon_running) {
			for (int i = 0; i < services_size; i++) {
				s = &services[i];
				service_stop(s);
			}
		}

		service_refresh_directory();
		stop_dummies();
		control_sockets();
		update_services();

		sleep(SV_CHECK_INTERVAL);

		cont = false;
		for (int i = 0; i < services_size; i++) {
			if (services[i].state != STATE_INACTIVE && services[i].state != STATE_ERROR)
				cont = true;
		}
	} while (cont);

	printf(":: terminating\n");

	printf(":: all services stopped\n");

cleanup:

	close(service_dir);
	close(null_fd);

	signal(SIGPIPE, SIG_DFL);
	signal(SIGCHLD, SIG_DFL);
	return 0;
}
