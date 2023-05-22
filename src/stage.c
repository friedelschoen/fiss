#include "config.h"
#include "service.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

static const char* stage_exec[] = {
	[0] = SV_START_EXEC,
	[2] = SV_STOP_EXEC
};


void service_handle_stage(int stage) {
	if (stage != 0 && stage != 2)
		return;

	// stage = 0 | 2
	int      pid, ttyfd, exitstat;
	sigset_t ss;
	while ((pid = fork()) == -1) {
		print_error("error: unable to fork for stage1: %s\n");
		sleep(5);
	}
	if (pid == 0) {
		/* child */

		if (stage == 0) {
			/* stage 1 gets full control of console */
			if ((ttyfd = open("/dev/console", O_RDWR)) == -1) {
				print_error("error: unable to open /dev/console: %s\n");
			} else {
				ioctl(ttyfd, TIOCSCTTY, NULL);    // make the controlling process
				dup2(ttyfd, 0);
				if (ttyfd > 2) close(ttyfd);
			}
		}

		sigblock_all(true);


		struct sigaction sigact = { 0 };
		sigact.sa_handler       = SIG_DFL;
		sigaction(SIGCHLD, &sigact, NULL);
		sigaction(SIGINT, &sigact, NULL);

		sigact.sa_handler = SIG_IGN;
		sigaction(SIGCONT, &sigact, NULL);

		printf("enter stage %d\n", stage);
		execl(stage_exec[stage], stage_exec[stage], NULL);
		print_error("error: unable to exec stage %d: %s\n", stage);
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
			print_error("warn: waitpid failed: %s");
			sleep(5);
		}

		reclaim_console();

		if (child == pid && stage == 0) {
			if (!WIFEXITED(exitstat) || WEXITSTATUS(exitstat) != 0) {
				if (WIFSIGNALED(exitstat)) {
					/* this is stage 1 */
					fprintf(stderr, "stage 1 failed: skip stage 2\n");
					daemon_running = false;
					break;
				}
			}
			printf("leave stage 1\n");
			break;
		}
		if (child <= 0) {
			/* collect terminated children */

			dont_wait = true;
			continue;
		}

		/* sig? */
		if (sig != SIGCONT && sig != SIGINT) {
			continue;
		}

		fprintf(stderr, "warn: signals only work in stage 2, ignoring...\n");
	}
}
