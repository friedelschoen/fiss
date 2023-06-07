#include "config.h"
#include "message.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/reboot.h>
#include <sys/wait.h>
#include <unistd.h>


char* stage[][4] = {
	{ SV_START_EXEC, NULL },
	{ SV_SUPERVISE_EXEC, SV_SERVICE_DIR, SV_RUNLEVEL_DEFAULT, NULL },
	{ SV_STOP_EXEC, NULL },
};

pipe_t selfpipe;
int    sigc = 0;
int    sigi = 0;

void sig_cont_handler(int signo) {
	(void) signo;

	sigc++;
	write(selfpipe.write, "", 1);
}

void sig_int_handler(int signo) {
	(void) signo;

	sigi++;
	write(selfpipe.write, "", 1);
}

void sig_child_handler(int signo) {
	(void) signo;

	write(selfpipe.write, "", 1);
}


int main(int argc, char** argv) {
	int              pid;
	int              wstat;
	struct pollfd    pollst;
	int              ttyfd;
	char             ch;
	sigset_t         ss;
	struct sigaction sa = { 0 };


	if (getpid() != 1) {
		if (argc != 2 || argv[1][1] != '\0' || (argv[1][0] != '0' && argv[1][0] != '6'))
			print_usage_exit(PROG_FINIT, 1);

		if (kill(1, argv[1][0] == '0' ? SIGCONT : SIGINT) == -1) {
			fprintf(stderr, "unable to signal init: %s\n", strerror(errno));
			return 1;
		}
		return 0;
	}

	setsid();

	sigemptyset(&ss);
	sigaddset(&ss, SIGALRM);
	sigaddset(&ss, SIGCHLD);
	sigaddset(&ss, SIGCONT);
	sigaddset(&ss, SIGHUP);
	sigaddset(&ss, SIGINT);
	sigaddset(&ss, SIGPIPE);
	sigaddset(&ss, SIGTERM);
	sigprocmask(SIG_BLOCK, &ss, NULL);

	sa.sa_handler = sig_child_handler;
	sigaction(SIGCHLD, &sa, NULL);

	sa.sa_handler = sig_cont_handler;
	sigaction(SIGCONT, &sa, NULL);

	sa.sa_handler = sig_int_handler;
	sigaction(SIGINT, &sa, NULL);


	/* console */
	if ((ttyfd = open("/dev/console", O_WRONLY)) != -1) {
		dup2(ttyfd, 0);
		dup2(ttyfd, 1);
		dup2(ttyfd, 2);
		if (ttyfd > 2)
			close(ttyfd);
	}

	/* create selfpipe */
	while (pipe((int*) &selfpipe) == -1) {
		fprintf(stderr, "unable to create selfpipe, pausing: %s\n", strerror(errno));
		sleep(5);
	}

	fd_set_flag(selfpipe.read, O_NONBLOCK);
	fd_set_flag(selfpipe.write, O_NONBLOCK);

#if RB_DISABLE_CAD == 0
	/* activate ctrlaltdel handling, glibc, dietlibc */
	reboot(RB_DISABLE_CAD);
#endif

	/* runit */
	for (int st = 0; st < 3; st++) {
		while ((pid = fork()) == -1) {
			fprintf(stderr, "unable to fork for \"%s\", pausing: %s\n", stage[st][0], strerror(errno));
			sleep(5);
		}
		if (pid == 0) {
			/* child */

			/* stage 1 gets full control of console */
			if (st == 0) {
				if ((ttyfd = open("/dev/console", O_RDWR)) != -1) {
					ioctl(ttyfd, TIOCSCTTY, (char*) 0);
					dup2(ttyfd, 0);
					if (ttyfd > 2)
						close(ttyfd);
				} else {
					fprintf(stderr, "warn: unable to open /dev/console: %s\n", strerror(errno));
				}
			} else {
				setsid();
			}

			sigemptyset(&ss);
			sigaddset(&ss, SIGALRM);
			sigaddset(&ss, SIGCHLD);
			sigaddset(&ss, SIGCONT);
			sigaddset(&ss, SIGHUP);
			sigaddset(&ss, SIGINT);
			sigaddset(&ss, SIGPIPE);
			sigaddset(&ss, SIGTERM);
			sigprocmask(SIG_UNBLOCK, &ss, (sigset_t*) 0);

			sa.sa_handler = SIG_DFL;
			sigaction(SIGCHLD, &sa, NULL);
			sa.sa_handler = SIG_IGN;
			sigaction(SIGINT, &sa, NULL);
			sigaction(SIGCONT, &sa, NULL);

			execv(stage[st][0], stage[st]);
			fprintf(stderr, "unable to exec child '%s': %s", stage[st][0], strerror(errno));
			_exit(1);
		}

		pollst.fd     = selfpipe.read;
		pollst.events = POLL_IN;

		int child;

	do_poll:

		sigemptyset(&ss);
		sigaddset(&ss, SIGCHLD);
		sigaddset(&ss, SIGCONT);
		sigaddset(&ss, SIGINT);
		sigprocmask(SIG_UNBLOCK, &ss, (sigset_t*) 0);

		poll(&pollst, 1, 14000);

		sigprocmask(SIG_BLOCK, &ss, (sigset_t*) 0);

		while (read(selfpipe.read, &ch, 1) == 1) {}

		child = waitpid(pid, &wstat, WNOHANG);

		if (child == -1) {
			fprintf(stderr, "cannot wait for %s, pausing: %s", stage[st][0], strerror(errno));
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
			if (!WIFEXITED(wstat) || WEXITSTATUS(wstat) != 0) {
				fprintf(stderr, "%s failed\n", stage[st][0]);
				if (st == 0) {
					/* this is stage 1 */
					fprintf(stderr, "stage 1 failed, starting emergency runlevel");
					stage[1][2] = "emergency";
					break;
				} else if (st == 1) {
					fprintf(stderr, "killing all processes in stage 2 and retry...\n");
					kill(-pid, 9);
					sleep(5);
					st--;
					break;
				}
			}
			fprintf(stderr, "leaving stage %d\n", st + 1);
		} else if (child != 0) {
			write(selfpipe.write, "", 1);
			goto do_poll;
		} else if (!sigc && !sigi) {
			goto do_poll;
		} else if (st != 1) {
			fprintf(stderr, "signals only work in stage 2\n");
			//			sigc = sigi = 0;
			goto do_poll;
		} else {

			kill(pid, SIGTERM);
			for (int i = 0; i < 5; i++) {
				if ((child = waitpid(-1, &wstat, WNOHANG)) == pid) {
					// stage terminated!
					pid = 0;
					break;
				} else if (child == -1) {
					fprintf(stderr, "waiting for terminated stage 2 failed: %s\n", strerror(errno));
					sleep(1);
				}
			}
			if (pid) {
				kill(pid, 9);
				if (waitpid(pid, &wstat, 0) == -1)
					fprintf(stderr, "waiting for killed stage 2 failed: %s\n", strerror(errno));
			}
		}
	}

	// xxxx

	close(selfpipe.read);
	close(selfpipe.write);

	/* reget stderr */
	if ((ttyfd = open("/dev/console", O_WRONLY)) != -1) {
		dup2(ttyfd, 1);
		dup2(ttyfd, 2);
		if (ttyfd > 2)
			close(ttyfd);
	}

#ifdef RB_AUTOBOOT
	/* fallthrough stage 3 */
	fprintf(stderr, "sending KILL signal to all processes...");
	kill(-1, SIGKILL);

	if ((pid = fork()) >= 0) {
		if (sigi) {    // wants reboot
			fprintf(stderr, "system reboot\n");
			sync();
			reboot(RB_AUTOBOOT);
		} else {
#	ifdef RB_POWER_OFF
			fprintf(stderr, "system poweroff\n");
			sync();
			reboot(RB_POWER_OFF);
			sleep(2);
#	endif
			fprintf(stderr, "system halt\n");
			sync();
#	if defined(RB_HALT_SYSTEM)
			reboot(RB_HALT_SYSTEM);
#	elif defined(RB_HALT)
			reboot(RB_HALT);
#	else
			reboot(RB_AUTOBOOT);
#	endif
		}
		if (pid == 0)
			_exit(0);
	} else {
		sigemptyset(&ss);
		sigaddset(&ss, SIGCHLD);
		sigprocmask(SIG_UNBLOCK, &ss, (sigset_t*) 0);

		while (waitpid(pid, NULL, 0) == -1) {}
	}
#endif

	sigemptyset(&ss);

	for (;;)
		sigsuspend(&ss);
}
