#include "service.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>


void sigblock_all(bool unblock) {
	sigset_t ss;
	sigemptyset(&ss);
	sigaddset(&ss, SIGALRM);
	sigaddset(&ss, SIGCHLD);
	sigaddset(&ss, SIGCONT);
	sigaddset(&ss, SIGHUP);
	sigaddset(&ss, SIGINT);
	sigaddset(&ss, SIGPIPE);
	sigaddset(&ss, SIGTERM);
	sigprocmask(unblock, &ss, NULL);
}

void handle_stage1() {
	int		 pid, ttyfd, exitstat;
	sigset_t ss;
	while ((pid = fork()) == -1) {
		print_error("unable to fork for stage1");
		sleep(5);
	}
	if (pid == 0) {
		/* child */

		/* stage 1 gets full control of console */
		if ((ttyfd = open("/dev/console", O_RDWR)) == -1) {
			print_error("unable to open /dev/console");
		} else {
			ioctl(ttyfd, TIOCSCTTY, NULL);	  // make the controlling process
			dup2(ttyfd, 0);
			if (ttyfd > 2) close(ttyfd);
		}

		sigblock_all(true);


		struct sigaction sigact = { 0 };
		sigact.sa_handler		= SIG_DFL;
		sigaction(SIGCHLD, &sigact, NULL);
		sigaction(SIGINT, &sigact, NULL);

		sigact.sa_handler = SIG_IGN;
		sigaction(SIGCONT, &sigact, NULL);

		printf("enter stage1\n");
		execl(SV_START_EXEC, SV_START_EXEC, NULL);
		print_error("unable to exec stage1");
		_exit(1);
	}
	bool dont_wait = false;
	for (;;) {
		int child;
		int sig = 0;

		if (!dont_wait) {
			sigemptyset(&ss);
			sigaddset(&ss, SIGCHLD);
			sigaddset(&ss, SIGCONT);
			sigaddset(&ss, SIGINT);

			sigwait(&ss, &sig);
		}
		dont_wait = false;

		do {
			child = waitpid(-1, &exitstat, WNOHANG);
		} while (child > 0 && child != pid);

		if (child == -1) {
			print_error("waitpid failed, pausing");
			sleep(5);
		}

		/* reget stderr */
		if ((ttyfd = open("/dev/console", O_WRONLY)) != -1) {
			dup2(ttyfd, 1);
			dup2(ttyfd, 2);
			if (ttyfd > 2)
				close(ttyfd);
		}

		if (child == pid) {
			if (!WIFEXITED(exitstat) || WEXITSTATUS(exitstat) != 0) {
				printf("child failed\n");
				if (WIFSIGNALED(exitstat)) {
					/* this is stage 1 */
					printf("leave stage 1\n");
					printf("skipping stage 2\n");
					daemon_running = false;
					break;
				}
			}
			printf("leave stage1\n");
			break;
		}
		if (child != 0) {
			/* collect terminated children */

			dont_wait = true;
			continue;
		}

		/* sig? */
		if (sig != SIGCONT && sig != SIGINT) {
			continue;
		}

		printf("signals only work in stage 2\n");
	}
}


void handle_stage3() {
	int		 pid, ttyfd, exitstat;
	sigset_t ss;
	while ((pid = fork()) == -1) {
		print_error("unable to fork for state3");
		sleep(5);
	}
	if (pid == 0) {
		/* child */

		setsid();

		sigblock_all(true);


		struct sigaction sigact = { 0 };
		sigact.sa_handler		= SIG_DFL;
		sigaction(SIGCHLD, &sigact, NULL);
		sigaction(SIGINT, &sigact, NULL);

		sigact.sa_handler = SIG_IGN;
		sigaction(SIGCONT, &sigact, NULL);

		printf("enter stage3\n");
		execl(SV_STOP_EXEC, SV_STOP_EXEC, NULL);
		print_error("unable to exec stage3");
		_exit(1);
	}
	bool dont_wait = false;
	for (;;) {
		int child;
		int sig;

		if (!dont_wait) {
			sigemptyset(&ss);
			sigaddset(&ss, SIGCHLD);
			sigaddset(&ss, SIGCONT);
			sigaddset(&ss, SIGINT);

			sigwait(&ss, &sig);
		}
		dont_wait = false;

		do {
			child = waitpid(-1, &exitstat, WNOHANG);
		} while (child > 0 && child != pid);

		if (child == -1) {
			print_error("waitpid failed, pausing");
			sleep(5);
		}

		/* reget stderr */
		if ((ttyfd = open("/dev/console", O_WRONLY)) != -1) {
			dup2(ttyfd, 1);
			dup2(ttyfd, 2);
			if (ttyfd > 2)
				close(ttyfd);
		}

		if (child == pid) {
			if (!WIFEXITED(exitstat) || WEXITSTATUS(exitstat) != 0) {
				printf("child failed\n");
			}
			printf("leave stage: stage3\n");
			break;
		}
		if (child != 0) {
			/* collect terminated children */
			dont_wait = true;
			continue;
		}

		/* sig? */
		if (sig != SIGCONT && sig != SIGINT) {
			continue;
		}
		printf("signals only work in stage 2\n");
	}
}
