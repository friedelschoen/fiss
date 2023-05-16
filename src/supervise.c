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

	service_check_state(s, WIFSIGNALED(status), WIFSIGNALED(status) ? WTERMSIG(status) : WEXITSTATUS(status));
}

static void check_deaths(void) {
	service_t* s;
	for (int i = 0; i < services_size; i++) {
		s = &services[i];
		if (s->state == STATE_ACTIVE_PID) {
			if (kill(s->pid, 0) == -1 && errno == ESRCH)
				service_check_state(s, false, 0);
		}
	}
}

static void check_services(void) {
	service_t* s;
	for (int i = 0; i < services_size; i++) {
		s = &services[i];
		if (s->state == STATE_DEAD)
			continue;
		if (service_need_restart(s)) {
			if (s->state == STATE_INACTIVE) {
				service_start(s, NULL);
			}
		} else {
			if (s->state != STATE_INACTIVE) {
				service_stop(s, NULL);
			}
		}
	}
}

static void accept_socket(void) {
	int client_fd;
	if ((client_fd = accept(control_socket, NULL, NULL)) == -1) {
		if (errno == EWOULDBLOCK) {
			sleep(SV_ACCEPT_INTERVAL);
		} else {
			print_error("error: cannot accept client from control-socket: %s\n");
		}
	} else {
		service_handle_socket(client_fd);
	}
}

static void control_sockets(void) {
	service_t* s;
	char       cmd;
	for (int i = 0; i < services_size; i++) {
		s = &services[i];

		while (read(s->control, &cmd, 1) == 1) {
			service_handle_command_runit(s, cmd);
		}
	}
}

int service_supervise(const char* service_dir_, const char* runlevel_, bool force_socket) {
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

	umask(0002);

	char socket_path[PATH_MAX];
	snprintf(socket_path, PATH_MAX, SV_CONTROL_SOCKET, runlevel);

	if ((null_fd = open("/dev/null", O_RDWR)) == -1) {
		print_error("error: cannot open /dev/null: %s\n");
		null_fd = 1;
	}

	struct stat socket_stat;
	if (force_socket) {
		if (unlink(socket_path) == -1 && errno != ENOENT) {
			print_error("error: cannot unlink socket: %s\n");
		}
	} else if (stat(socket_path, &socket_stat) != -1 && S_ISREG(socket_stat.st_mode)) {
		printf("error: %s exist and is locking supervision,\nrun this program with '-f' flag if you are sure no other superviser is running.", socket_path);
		return 1;
	}
	// create socket
	if ((control_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		print_error("error: cannot create socket: %s\n");
		return 1;
	}

	// bind socket to address
	struct sockaddr_un addr = { 0 };
	addr.sun_family         = AF_UNIX;
	strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path));
	if (bind(control_socket, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
		print_error("error: cannot bind %s to socket: %s\n", socket_path);
		return 1;
	}

	// listen for connections
	if (listen(control_socket, 5) == -1) {
		print_error("error: cannot listen to control socket: %s\n");
		return 1;
	}

	int sockflags = fcntl(control_socket, F_GETFL, 0);
	if (sockflags == -1) {
		print_error("warn: fcntl-getflags on control-socket failed: %s\n");
	} else if (fcntl(control_socket, F_SETFL, sockflags | O_NONBLOCK) == -1) {
		print_error("warn: fcntl-setflags on control-socket failed: %s\n");
	}

	printf(":: starting services on '%s'\n", runlevel);

	if (service_refresh() < 0)
		return 1;

	printf(":: started services\n");

	// accept connections and handle requests
	while (daemon_running) {
		check_deaths();
		service_refresh();
		check_services();
		control_sockets();
		accept_socket();
	}

	close(control_socket);

	if (unlink(socket_path) == -1 && errno != ENOENT) {
		print_error("error: cannot unlink socket: %s\n");
	}

	printf(":: terminating\n");

	service_t* s;
	for (int i = 0; i < services_size; i++) {
		s = &services[i];
		service_stop(s, NULL);
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
			service_send(&services[i], SIGKILL);
		}
	}

	printf(":: all services stopped\n");

	signal(SIGPIPE, SIG_DFL);
	signal(SIGCHLD, SIG_DFL);
	signal(SIGINT, SIG_DFL);
	signal(SIGCONT, SIG_DFL);
	return 0;
}
