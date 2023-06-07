#include "stage.h"

#include "config.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>


static char* stage_exec[][4] = {
	[0] = { SV_START_EXEC, NULL },
	[1] = { SV_SUPERVISE_EXEC, SV_SERVICE_DIR, SV_RUNLEVEL_DEFAULT, NULL },
	[2] = { SV_STOP_EXEC, NULL },
};


bool handle_stage(int stage) {
	int      pid, ttyfd, exitstat, sig = 0;
	sigset_t ss;
	bool     cont = true;

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

		printf("enter stage %d\n", stage);
		execv(stage_exec[stage][0], stage_exec[stage]);
		print_error("error: unable to exec stage %d: %s\n", stage);
		_exit(1);
	}

	sigemptyset(&ss);
	sigaddset(&ss, SIGCHLD);
	sigaddset(&ss, SIGUSR1);
	sigaddset(&ss, SIGCONT);

	sigwait(&ss, &sig);

	if (stage == 1 && sig != SIGCHLD)
		kill(pid, SIGTERM);

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
				cont = false;
			}
		}
		printf("leave stage 1\n");
	}

	return cont;
}
