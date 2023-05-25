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


bool daemon_running = true;

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

static void check_services(void) {
	service_t* s;
	for (int i = 0; i < services_size; i++) {
		s = &services[i];
		if (s->state == STATE_DEAD)
			continue;
		if (service_need_restart(s)) {
			if (s->state == STATE_INACTIVE) {
				service_start(s);
				s->status_change = time(NULL);
				service_update_status(s);
			}
		} else {
			if (s->state != STATE_INACTIVE) {
				service_stop(s);
				s->status_change = time(NULL);
				service_update_status(s);
			}
		}
	}
}


static void control_sockets(void) {
	service_t* s;
	char       cmd, chr;
	bool       read_signo = false;
	for (int i = 0; i < services_size; i++) {
		s = &services[i];
		while (read(s->control, &chr, 1) == 1) {
			printf("handling '%c' from %s\n", chr, s->name);
			if (read_signo) {
				service_handle_command(s, cmd, chr);
				read_signo = false;
			} else if (chr == X_XUP || chr == X_XDOWN) {
				cmd        = chr;
				read_signo = true;
			} else {
				service_handle_command(s, cmd, 0);
			}
		}
	}
}

int service_supervise(const char* service_dir_, const char* runlevel_) {
	struct sigaction sigact = { 0 };
	sigact.sa_handler       = signal_child;
	sigaction(SIGCHLD, &sigact, NULL);
	sigact.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sigact, NULL);

	strncpy(runlevel, runlevel_, SV_NAME_MAX);
	service_dir_path = service_dir_;
	if ((service_dir = open(service_dir_, O_DIRECTORY)) == -1) {
		print_error("error: cannot open directory %s: %s\n", service_dir_);
		return 1;
	}

	//	setenv("SERVICE_RUNLEVEL", runlevel, true);

	if ((null_fd = open("/dev/null", O_RDWR)) == -1) {
		print_error("error: cannot open /dev/null: %s\n");
		null_fd = 1;
	}

	printf(":: starting services on '%s'\n", runlevel);

	// accept connections and handle requests
	while (daemon_running) {
		service_refresh_directory();
		check_services();
		control_sockets();
		sleep(SV_CHECK_INTERVAL);
	}


	printf(":: terminating\n");

	service_t* s;
	for (int i = 0; i < services_size; i++) {
		s = &services[i];
		service_stop(s);
	}

	time_t start = time(NULL);
	int    running;
	do {
		sleep(1);    // sleep for one second
		running = 0;
		for (int i = 0; i < services_size; i++) {
			if (services[i].state != STATE_INACTIVE)
				running++;
		}
		printf(":: %d running...\r", running);
	} while (running > 0 && (time(NULL) - start) < SV_STOP_TIMEOUT);

	printf("\n");

	for (int i = 0; i < services_size; i++) {
		if (services[i].pid) {
			printf(":: killing %s\n", services[i].name);
			service_kill(&services[i], SIGKILL);
		}
	}

	printf(":: all services stopped\n");

	signal(SIGPIPE, SIG_DFL);
	signal(SIGCHLD, SIG_DFL);
	signal(SIGINT, SIG_DFL);
	signal(SIGCONT, SIG_DFL);
	return 0;
}
