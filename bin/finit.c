// +objects: message.o util.o supervise.o service.o start.o stop.o
// +objects: register.o handle_exit.o handle_command.o
// +objects: encode.o parse.o dependency.o pattern.o status.o stage.o
// +flags: -static

#include "config.h"
#include "message.h"
#include "service.h"
#include "stage.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/wait.h>
#include <unistd.h>


static bool do_reboot;

static int handle_initctl(int argc, const char** argv) {
	int sig;

	if (argc != 2 || argv[1][1] != '\0' || (argv[1][0] != '0' && argv[1][0] != '6')) {
		print_usage_exit(PROG_FINIT, 1);
	}
	if (getuid() != 0) {
		fprintf(stderr, "error: can only be run as root...\n");
		return 1;
	}
	sig = argv[1][0] == '0' ? SIGTERM : SIGINT;
	if (kill(1, sig) == -1) {
		print_error("error: unable to kill init: %s\n");
		return 1;
	}
	return 0;
}

static void signal_interrupt(int signum) {
	daemon_running = false;
	do_reboot      = signum == SIGINT;
}


int main(int argc, const char** argv) {
	sigset_t ss;
	pid_t    pid;

	if (getpid() != 1) {
		return handle_initctl(argc, argv);
	}
	setsid();

	sigblock_all(false);

	reclaim_console();

	// disable ctrl-alt-delete
	reboot(0);

	printf("booting...\n");

	// stage 1
	handle_stage(0);


	// stage 2
	if (daemon_running) {    // stage1 succeed
		struct sigaction sigact = { 0 };

		sigblock_all(true);

		sigact.sa_handler = signal_interrupt;
		sigaction(SIGTERM, &sigact, NULL);
		sigaction(SIGINT, &sigact, NULL);

		service_supervise(SV_SERVICE_DIR, SV_RUNLEVEL_DEFAULT);
		sigblock_all(false);
	}

	// stage 3
	handle_stage(2);

#ifdef RB_AUTOBOOT
	/* fallthrough stage 3 */
	printf("sending KILL signal to all processes...\n");
	kill(-1, SIGKILL);

	if ((pid = fork()) <= 0) {
		if (do_reboot) {
			printf("system reboot\n");
			sync();
			reboot(RB_AUTOBOOT);
		} else {
#	if defined(RB_POWER_OFF)
			printf("system power off\n");
			sync();
			reboot(RB_POWER_OFF);
			sleep(2);
#	elif defined(RB_HALT_SYSTEM)
			printf("system halt\n");
			sync();
			reboot(RB_HALT_SYSTEM);
#	elif define(RB_HALT)
			printf("system halt\n");
			sync();
			reboot(RB_HALT);
#	else
			printf("system reboot\n");
			sync();
			reboot(RB_AUTOBOOT);
#	endif
		}
		if (pid == 0)
			_exit(0);
	} else {
		sigemptyset(&ss);
		sigaddset(&ss, SIGCHLD);
		sigprocmask(SIG_UNBLOCK, &ss, NULL);

		while (waitpid(pid, NULL, 0) != -1) {}
	}
#endif

	sigfillset(&ss);
	for (;;)
		sigsuspend(&ss);

	/* not reached */
	printf("exit.\n");
	return 0;
}
