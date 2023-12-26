// +objects: parse.o util.o

#include "parse.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/resource.h>


const char* current_prog(void) {
	return "chpst";
}

void limit(int what, rlim_t l) {
	struct rlimit r;

	if (getrlimit(what, &r) == -1)
		fprintf(stderr, "error: unable to getrlimit\n");

	if (l < 0) {
		r.rlim_cur = 0;
	} else if (l > r.rlim_max)
		r.rlim_cur = r.rlim_max;
	else
		r.rlim_cur = l;

	if (setrlimit(what, &r) == -1)
		fprintf(stderr, "error: unable to setrlimit\n");
}


int main(int argc, char** argv) {
	int   opt, lockfd, lockflags, gid_len = 0;
	char *arg0 = NULL, *root = NULL, *cd = NULL, *lock = NULL, *exec = NULL;
	uid_t uid = 0;
	gid_t gid[61];
	long  limitd     = -2,
	     limits      = -2,
	     limitl      = -2,
	     limita      = -2,
	     limito      = -2,
	     limitp      = -2,
	     limitf      = -2,
	     limitc      = -2,
	     limitr      = -2,
	     limitt      = -2;
	long nicelevel   = 0;
	bool ssid        = false;
	bool closestd[3] = { false, false, false };

	if (streq(argv[0], "setuidgid") || streq(argv[0], "envuidgid")) {
		if (argc < 2) {
			fprintf(stderr, "%s <uid-gid> command...", argv[0]);
			return 1;
		}
		gid_len = parse_ugid(argv[1], &uid, gid);
		argv += 2, argc -= 2;
	} else if (streq(argv[0], "pgrphack")) {
		ssid = true;
		argv += 1, argc -= 1;
	} else if (streq(argv[0], "setlock")) {
		while ((opt = getopt(argc, argv, "+xXnN")) != -1) {
			switch (opt) {
				case 'n':
					lockflags = LOCK_EX | LOCK_NB;
					break;
				case 'N':
					lockflags = LOCK_EX;
					break;
				case 'x':
				case 'X':
					fprintf(stderr, "warning: '-%c' is ignored\n", optopt);
					break;
				case '?':
					fprintf(stderr, "%s [-xXnN] command...", argv[0]);
					return 1;
			}
		}
		argv += optind, argc -= optind;
		if (argc < 1) {
			fprintf(stderr, "%s [-xXnN] command...", argv[0]);
			return 1;
		}
		lock = argv[0];
		argv += 1, argc -= 1;
	} else if (streq(argv[0], "softlimit")) {
		while ((opt = getopt(argc, argv, "+a:c:d:f:l:m:o:p:r:s:t:")) != -1) {
			switch (opt) {
				case 'm':
					limits = limitl = limita = limitd = parse_long(optarg, "limit");
					break;
				case 'a':
					limita = parse_long(optarg, "limit");
					break;
				case 'd':
					limitd = parse_long(optarg, "limit");
					break;
				case 'o':
					limito = parse_long(optarg, "limit");
					break;
				case 'p':
					limitp = parse_long(optarg, "limit");
					break;
				case 'f':
					limitf = parse_long(optarg, "limit");
					break;
				case 'c':
					limitc = parse_long(optarg, "limit");
					break;
				case 'r':
					limitr = parse_long(optarg, "limit");
					break;
				case 't':
					limitt = parse_long(optarg, "limit");
					break;
				case 'l':
					limitl = parse_long(optarg, "limit");
					break;
				case 's':
					limits = parse_long(optarg, "limit");
					break;
				case '?':
					fprintf(stderr, "softlimit command...");
					return 1;
			}
		}
		argv += optind, argc -= optind;
	} else {
		if (!streq(argv[0], "chpst"))
			fprintf(stderr, "warning: program-name unsupported, asuming `chpst`\n");

		while ((opt = getopt(argc, argv, "+u:U:b:e:m:d:o:p:f:c:r:t:/:C:n:l:L:vP012V")) != -1) {
			switch (opt) {
				case 'u':
				case 'U':
					gid_len = parse_ugid(optarg, &uid, gid);
					break;
				case 'b':
					arg0 = optarg;
					break;
				case '/':
					root = optarg;
					break;
				case 'C':
					cd = optarg;
					break;
				case 'n':
					nicelevel = parse_long(optarg, "nice-level");
					break;
				case 'l':
					lock      = optarg;
					lockflags = LOCK_EX | LOCK_NB;
					break;
				case 'L':
					lock      = optarg;
					lockflags = LOCK_EX;
					break;
				case 'v':    // ignored
					break;
				case 'P':
					ssid = true;
					break;
				case '0':
				case '1':
				case '2':
					closestd[opt - '0'] = true;
					break;
				case 'm':
					limits = limitl = limita = limitd = parse_long(optarg, "limit");
					break;
				case 'd':
					limitd = parse_long(optarg, "limit");
					break;
				case 'o':
					limito = parse_long(optarg, "limit");
					break;
				case 'p':
					limitp = parse_long(optarg, "limit");
					break;
				case 'f':
					limitf = parse_long(optarg, "limit");
					break;
				case 'c':
					limitc = parse_long(optarg, "limit");
					break;
				case 'r':
					limitr = parse_long(optarg, "limit");
					break;
				case 't':
					limitt = parse_long(optarg, "limit");
					break;
				case 'e':
					fprintf(stderr, "warning: '-%c' is ignored\n", optopt);
					break;
				case '?':
					fprintf(stderr, "usage\n");
					return 1;
			}
		}
		argv += optind, argc -= optind;
	}

	if (argc == 0) {
		fprintf(stderr, "%s: command required\n", argv[0]);
		return 1;
	}

	if (ssid) {
		setsid();
	}

	if (uid) {
		setgroups(gid_len, gid);
		setgid(gid[0]);
		setuid(uid);
		// $EUID
	}

	if (root) {
		if (chroot(root) == -1)
			print_errno("unable to change root directory: %s\n");

		// chdir to '/', otherwise the next command will complain 'directory not found'
		chdir("/");
	}

	if (cd) {
		chdir(cd);
	}

	if (nicelevel != 0) {
		if (nice(nicelevel) == -1)
			print_errno("unable to set nice level: %s\n");
	}

	if (limitd >= -1) {
#ifdef RLIMIT_DATA
		limit(RLIMIT_DATA, limitd);
#else
		if (verbose)
			fprintf(stderr, "system does not support RLIMIT_DATA\n");
#endif
	}
	if (limits >= -1) {
#ifdef RLIMIT_STACK
		limit(RLIMIT_STACK, limits);
#else
		if (verbose)
			fprintf(stderr, "system does not support RLIMIT_STACK\n");
#endif
	}
	if (limitl >= -1) {
#ifdef RLIMIT_MEMLOCK
		limit(RLIMIT_MEMLOCK, limitl);
#else
		if (verbose)
			fprintf(stderr, "system does not support RLIMIT_MEMLOCK\n");
#endif
	}
	if (limita >= -1) {
#ifdef RLIMIT_VMEM
		limit(RLIMIT_VMEM, limita);
#else
#	ifdef RLIMIT_AS
		limit(RLIMIT_AS, limita);
#	else
		if (verbose)
			fprintf(stderr, "system does neither support RLIMIT_VMEM nor RLIMIT_AS\n");
#	endif
#endif
	}
	if (limito >= -1) {
#ifdef RLIMIT_NOFILE
		limit(RLIMIT_NOFILE, limito);
#else
#	ifdef RLIMIT_OFILE
		limit(RLIMIT_OFILE, limito);
#	else
		if (verbose)
			fprintf(stderr, "system does neither support RLIMIT_NOFILE nor RLIMIT_OFILE\n");
#	endif
#endif
	}
	if (limitp >= -1) {
#ifdef RLIMIT_NPROC
		limit(RLIMIT_NPROC, limitp);
#else
		if (verbose)
			fprintf(stderr, "system does not support RLIMIT_NPROC\n");
#endif
	}
	if (limitf >= -1) {
#ifdef RLIMIT_FSIZE
		limit(RLIMIT_FSIZE, limitf);
#else
		if (verbose)
			fprintf(stderr, "system does not support RLIMIT_FSIZE\n");
#endif
	}
	if (limitc >= -1) {
#ifdef RLIMIT_CORE
		limit(RLIMIT_CORE, limitc);
#else
		if (verbose)
			fprintf(stderr, "system does not support RLIMIT_CORE\n");
#endif
	}
	if (limitr >= -1) {
#ifdef RLIMIT_RSS
		limit(RLIMIT_RSS, limitr);
#else
		if (verbose)
			fprintf(stderr, "system does not support RLIMIT_RSS\n");
#endif
	}
	if (limitt >= -1) {
#ifdef RLIMIT_CPU
		limit(RLIMIT_CPU, limitt);
#else
		if (verbose)
			fprintf(stderr, "system does not support RLIMIT_CPU\n");
#endif
	}

	if (lock) {
		if ((lockfd = open(lock, O_WRONLY | O_APPEND)) == -1)
			print_errno("unable to open lock: %s\n");

		if (flock(lockfd, lockflags) == -1)
			print_errno("unable to lock: %s\n");
	}

	if (closestd[0] && close(0) == -1)
		print_errno("unable to close stdin: %s\n");
	if (closestd[1] && close(1) == -1)
		print_errno("unable to close stdout: %s\n");
	if (closestd[2] && close(2) == -1)
		print_errno("unable to close stderr: %s\n");

	exec = argv[0];
	if (arg0)
		argv[0] = arg0;

	execvp(exec, argv);
	print_errno("cannot execute: %s\n");
}
