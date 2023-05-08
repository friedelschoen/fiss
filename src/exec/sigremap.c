/*  Copyright (c) 2015 Yelp, Inc.
    With modification 2023 Friedel Schon

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
    */

/*
 * sigremap is a simple wrapper program designed to run as PID 1 and pass
 * signals to its children.
 *
 * Usage:
 *   ./sigremap python -c 'while True: pass'
 *
 * To get debug output on stderr, run with '-v'.
 */

#include "message.h"
#include "signame.h"
#include "util.h"

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

#define DEBUG(...)                        \
	do {                                  \
		if (debug)                        \
			fprintf(stderr, __VA_ARGS__); \
	} while (0)

#define set_signal_undefined(old, new) \
	if (signal_remap[old] == -1)       \
		signal_remap[old] = new;


// Signals we care about are numbered from 1 to 31, inclusive.
// (32 and above are real-time signals.)
// TODO: this is likely not portable outside of Linux, or on strange architectures
#define MAXSIG 31

// Indices are one-indexed (signal 1 is at index 1). Index zero is unused.
// User-specified signal rewriting.
int signal_remap[MAXSIG + 1] = { [0 ... MAXSIG] = -1 };
// One-time ignores due to TTY quirks. 0 = no skip, 1 = skip the next-received signal.
bool signal_temporary_ignores[MAXSIG + 1] = { [0 ... MAXSIG] = false };

pid_t child_pid  = -1;
bool  debug      = false;
bool  use_setsid = true;

void forward_signal(int signum) {
	if (signum >= 0 && signum <= MAXSIG && signal_remap[signum] != -1) {
		DEBUG("Translating signal %d to %d.\n", signum, signal_remap[signum]);
		signum = signal_remap[signum];
	}

	if (signum == 0) {
		DEBUG("Not forwarding signal %d to children (ignored).\n", signum);
		return;
	}

	kill(use_setsid ? -child_pid : child_pid, signum);
	DEBUG("Forwarded signal %d to children.\n", signum);
}

/*
 * The sigremap signal handler.
 *
 * The main job of this signal handler is to forward signals along to our child
 * process(es). In setsid mode, this means signaling the entire process group
 * rooted at our child. In non-setsid mode, this is just signaling the primary
 * child.
 *
 * In most cases, simply proxying the received signal is sufficient. If we
 * receive a job control signal, however, we should not only forward it, but
 * also sleep sigremap itself.
 *
 * This allows users to run foreground processes using sigremap and to
 * control them using normal shell job control features (e.g. Ctrl-Z to
 * generate a SIGTSTP and suspend the process).
 *
 * The libc manual is useful:
 * https://www.gnu.org/software/libc/manual/html_node/Job-Control-Signals.html
 *
 */
void handle_signal(int signum) {
	DEBUG("Received signal %d.\n", signum);

	if (signal_temporary_ignores[signum] == 1) {
		DEBUG("Ignoring tty hand-off signal %d.\n", signum);
		signal_temporary_ignores[signum] = 0;
	} else if (signum == SIGCHLD) {
		int   status, exit_status;
		pid_t killed_pid;
		while ((killed_pid = waitpid(-1, &status, WNOHANG)) > 0) {
			if (WIFEXITED(status)) {
				exit_status = WEXITSTATUS(status);
				DEBUG("A child with PID %d exited with exit status %d.\n", killed_pid, exit_status);
			} else {
				assert(WIFSIGNALED(status));
				exit_status = 128 + WTERMSIG(status);
				DEBUG("A child with PID %d was terminated by signal %d.\n", killed_pid, exit_status - 128);
			}

			if (killed_pid == child_pid) {
				kill(use_setsid ? -child_pid : child_pid, SIGTERM);    // send SIGTERM to any remaining children
				DEBUG("Child exited with status %d. Goodbye.\n", exit_status);
				exit(exit_status);
			}
		}
	} else {
		if (signum <= MAXSIG && signal_remap[signum] != -1) {
			DEBUG("Translating signal %d to %d.\n", signum, signal_remap[signum]);
			signum = signal_remap[signum];
		}

		kill(use_setsid ? -child_pid : child_pid, signum);
		DEBUG("Forwarded signal %d to children.\n", signum);

		if (signum == SIGTSTP || signum == SIGTTOU || signum == SIGTTIN) {
			DEBUG("Suspending self due to TTY signal.\n");
			kill(getpid(), SIGSTOP);
		}
	}
}

char** parse_command(int argc, char* argv[]) {
	int           opt;
	struct option long_options[] = {
		{ "single", no_argument, NULL, 's' },
		{ "verbose", no_argument, NULL, 'v' },
		{ "version", no_argument, NULL, 'V' },
		{ NULL, 0, NULL, 0 },
	};
	while ((opt = getopt_long(argc, argv, "+:hvVs", long_options, NULL)) != -1) {
		switch (opt) {
			case 'v':
				debug = true;
				break;
			case 'V':
				print_version_exit();
			case 'c':
				use_setsid = false;
				break;
			default:
				print_usage_exit(PROG_SIGREMAP, 1);
		}
	}

	argc -= optind, argv += optind;

	while (argc > 0) {
		char *old, *new;
		if ((new = strchr(argv[0], '=')) == NULL)
			break;

		old  = argv[0];
		*new = '\0';
		new ++;

		int oldsig, newsig;

		if ((oldsig = signame(old)) == -1) {
			fprintf(stderr, "error: invalid old signal '%s'\n", old);
			exit(1);
		}
		if ((newsig = signame(new)) == -1) {
			fprintf(stderr, "error: invalid new signal '%s'\n", new);
			exit(1);
		}
		signal_remap[oldsig] = newsig;

		argc--, argv++;
	}

	if (argc < 1) {
		print_usage_exit(PROG_SIGREMAP, 1);
	}

	if (use_setsid) {
		set_signal_undefined(SIGTSTP, SIGSTOP);
		set_signal_undefined(SIGTSTP, SIGTTOU);
		set_signal_undefined(SIGTSTP, SIGTTIN);
	}

	return &argv[optind];
}

// A dummy signal handler used for signals we care about.
// On the FreeBSD kernel, ignored signals cannot be waited on by `sigwait` (but
// they can be on Linux). We must provide a dummy handler.
// https://lists.freebsd.org/pipermail/freebsd-ports/2009-October/057340.html
void dummy(int signum) {
	(void) signum;
}

int main(int argc, char* argv[]) {
	char**   cmd = parse_command(argc, argv);
	sigset_t all_signals;
	sigfillset(&all_signals);
	sigprocmask(SIG_BLOCK, &all_signals, NULL);

	for (int i = 1; i <= MAXSIG; i++) {
		signal(i, dummy);
	}

	/*
	 * Detach sigremap from controlling tty, so that the child's session can
	 * attach to it instead.
	 *
	 * We want the child to be able to be the session leader of the TTY so that
	 * it can do normal job control.
	 */
	if (use_setsid) {
		if (ioctl(STDIN_FILENO, TIOCNOTTY) == -1) {
			DEBUG(
			    "Unable to detach from controlling tty (errno=%d %s).\n",
			    errno,
			    strerror(errno));
		} else {
			/*
			 * When the session leader detaches from its controlling tty via
			 * TIOCNOTTY, the kernel sends SIGHUP and SIGCONT to the process
			 * group. We need to be careful not to forward these on to the
			 * sigremap child so that it doesn't receive a SIGHUP and
			 * terminate itself (#136).
			 */
			if (getsid(0) == getpid()) {
				DEBUG("Detached from controlling tty, ignoring the first SIGHUP and SIGCONT we receive.\n");
				signal_temporary_ignores[SIGHUP]  = 1;
				signal_temporary_ignores[SIGCONT] = 1;
			} else {
				DEBUG("Detached from controlling tty, but was not session leader.\n");
			}
		}
	}

	child_pid = fork();
	if (child_pid < 0) {
		print_error("error: unable to fork: %s\n");
		return 1;
	} else if (child_pid == 0) {
		/* child */
		sigprocmask(SIG_UNBLOCK, &all_signals, NULL);
		if (use_setsid) {
			if (setsid() == -1) {
				print_error("error: unable to setsid: %s\n");
				exit(1);
			}

			if (ioctl(STDIN_FILENO, TIOCSCTTY, 0) == -1) {
				DEBUG(
				    "Unable to attach to controlling tty (errno=%d %s).\n",
				    errno,
				    strerror(errno));
			}
			DEBUG("setsid complete.\n");
		}
		execvp(cmd[0], cmd);

		// if this point is reached, exec failed, so we should exit nonzero
		print_error("error: unable to execute %s: %s\n", cmd[0]);
		_exit(2);
	}

	/* parent */
	DEBUG("Child spawned with PID %d.\n", child_pid);
	for (;;) {
		int signum;
		sigwait(&all_signals, &signum);
		handle_signal(signum);
	}
}
