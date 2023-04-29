#include "config.h"
#include "service.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/wait.h>
#include <unistd.h>


void sigblock_all(bool unblock);

int handle_initctl(int argc, const char** argv) {
	if (argc != 2 || argv[1][1] != '\0' || (argv[1][0] != '0' && argv[1][0] != '6')) {
		printf("Usage: %s <0|6>\n", argv[0]);
		return 1;
	}
	if (getuid() != 0) {
		printf("can only be run as root...\n");
		return 1;
	}
	int sig = argv[1][0] == '0' ? SIGTERM : SIGINT;
	if (kill(1, sig) == -1) {
		print_error("unable to kill init");
		return 1;
	}
	return 0;
}


void handle_stage1();
void handle_stage3();

static bool do_reboot;

static void signal_interrupt(int signum) {
	daemon_running = false;
	do_reboot	   = signum == SIGINT;
}


int main(int argc, const char** argv) {
	int		 ttyfd;
	sigset_t ss;

	if (getpid() != 1) {
		return handle_initctl(argc, argv);
	}
	setsid();

	sigblock_all(false);

	/* console */
	if ((ttyfd = open("/dev/console", O_WRONLY)) != -1) {
		dup2(ttyfd, 0);
		dup2(ttyfd, 1);
		dup2(ttyfd, 2);
		if (ttyfd > 2) close(ttyfd);
	}

	// disable ctrl-alt-delete
	reboot(0);

	printf("booting...\n");

	handle_stage1();

	if (daemon_running) {	 // stage1 succeed
		sigblock_all(true);

		struct sigaction sigact = { 0 };
		sigact.sa_handler		= signal_interrupt;
		sigaction(SIGTERM, &sigact, NULL);
		sigaction(SIGINT, &sigact, NULL);

		service_supervise(SV_SERVICE_DIR, SV_RUNLEVEL, true);
		sigblock_all(false);
	}

	handle_stage3();

	/* reget stderr */
	if ((ttyfd = open("/dev/console", O_WRONLY)) != -1) {
		dup2(ttyfd, 1);
		dup2(ttyfd, 2);
		if (ttyfd > 2) close(ttyfd);
	}

#ifdef RB_AUTOBOOT
	/* fallthrough stage 3 */
	printf("sending KILL signal to all processes...\n");
	kill(-1, SIGKILL);
	pid_t pid;

	if ((pid = fork()) <= 0) {
		if (do_reboot) {
			printf("system reboot\n");
			sync();
			reboot(RB_AUTOBOOT);
		} else {
#	ifdef RB_POWER_OFF
			printf("system power off\n");
			sync();
			reboot(RB_POWER_OFF);
			sleep(2);
#	endif
#	ifdef RB_HALT_SYSTEM
			printf("system halt\n");
			sync();
			reboot(RB_HALT_SYSTEM);
#	else
#		ifdef RB_HALT
			printf("system halt\n");
			sync();
			reboot(RB_HALT);
#		else
			printf("system reboot\n");
			sync();
			reboot(RB_AUTOBOOT);
#		endif
#	endif
		}
		if (pid == 0)
			_exit(0);
	} else {
		sigemptyset(&ss);
		sigaddset(&ss, SIGCHLD);
		sigprocmask(SIG_UNBLOCK, &ss, NULL);

		while (waitpid(pid, NULL, 0) == -1)
			;
	}
#endif

	sigfillset(&ss);
	for (;;)
		sigsuspend(&ss);

	/* not reached */
	printf("exit.\n");
	return 0;
}
