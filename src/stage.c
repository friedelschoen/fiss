#include "config.h"
#include "service.h"
#include "util.h"

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


void service_stage(int stage) {
	int              pid, ttyfd, exitstat, sig = 0;
	sigset_t         ss;
	struct sigaction sigact = { 0 };

	// stage = 0 | 2
	if (stage != 0 && stage != 2)
		return;

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


		sigact.sa_handler = SIG_DFL;
		sigaction(SIGCHLD, &sigact, NULL);
		sigaction(SIGINT, &sigact, NULL);

		sigact.sa_handler = SIG_IGN;
		sigaction(SIGCONT, &sigact, NULL);

		printf("enter stage %d\n", stage);
		execl(stage_exec[stage], stage_exec[stage], NULL);
		print_error("error: unable to exec stage %d: %s\n", stage);
		_exit(1);
	}

	sigemptyset(&ss);
	sigaddset(&ss, SIGCHLD);
	sigaddset(&ss, SIGCONT);
	sigaddset(&ss, SIGINT);

	sigwait(&ss, &sig);

	if (waitpid(pid, &exitstat, 0) == -1) {
		print_error("warn: waitpid failed: %s");
		sleep(5);
	}

	reclaim_console();

	if (stage == 0) {
		if (!WIFEXITED(exitstat) || WEXITSTATUS(exitstat) != 0) {
			if (WIFSIGNALED(exitstat)) {
				/* this is stage 1 */
				fprintf(stderr, "stage 1 failed: skip stage 2\n");
				daemon_running = false;
			}
		}
		printf("leave stage 1\n");
	}
}
