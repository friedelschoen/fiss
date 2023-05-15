#include "user_group.h"
#include "util.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/resource.h>
#include <unistd.h>

static long parse_long(const char* str) {
	char* end;
	long  l = strtol(str, &end, 10);
	if (*end != '\0') {
		fprintf(stderr, "error: invalid limit '%s'\n", optarg);
		exit(1);
	}
	return l;
}

void limit(int what, long l) {
	struct rlimit r;

	if (getrlimit(what, &r) == -1)
		print_error("unable to getrlimit(): %s\n");
	if ((l < 0) || (l > (long int) r.rlim_max))
		r.rlim_cur = r.rlim_max;
	else
		r.rlim_cur = l;
	if (setrlimit(what, &r) == -1)
		print_error("unable to setrlimit(): %s\n");
}

void slock(const char* path, bool d) {
	int fd;

	if ((fd = open(path, O_WRONLY | O_APPEND)) == -1)
		print_error("unable to open lock: %s\n");

	if (d) {
		if (flock(fd, LOCK_EX) == -1)
			print_error("unable to lock: %s\n");
	} else if (flock(fd, LOCK_EX | LOCK_NB) == -1)
		print_error("unable to lock: %s\n");
}


int main(int argc, char** argv) {
	int   opt;
	char *arg0 = NULL, *root = NULL, *cd = NULL, *lock = NULL;
	uid_t uid = 0;
	gid_t gid[61];
	int   gid_len   = 0;
	long  nicelevel = 0,
	     limitd     = -2,
	     limits     = -2,
	     limitl     = -2,
	     limita     = -2,
	     limito     = -2,
	     limitp     = -2,
	     limitf     = -2,
	     limitc     = -2,
	     limitr     = -2,
	     limitt     = -2;
	bool lockdelay, ssid = false;
	bool closestd[3] = { false, false, false };

	while ((opt = getopt(argc, argv, "+u:U:b:e:m:d:o:p:f:c:r:t:/:C:n:l:L:vP012V")) != -1) {
		switch (opt) {
			case 'u':
			case 'U':
				gid_len = parse_ugid(optarg, &uid, gid);
				break;
			case 'b':
				arg0 = optarg;
				break;
			case 'e':    // ignored
				fprintf(stderr, "`envdir` is ignored\n");
				break;
			case 'd':
				limitd = parse_long(optarg);
				break;
			case 'o':
				limito = parse_long(optarg);
				break;
			case 'p':
				limitp = parse_long(optarg);
				break;
			case 'f':
				limitf = parse_long(optarg);
				break;
			case 'c':
				limitc = parse_long(optarg);
				break;
			case 'r':
				limitr = parse_long(optarg);
				break;
			case 't':
				limitt = parse_long(optarg);
				break;
			case 'm':
				limits = limitl = limita = limitd = parse_long(optarg);
				break;
			case '/':
				root = optarg;
				break;
			case 'C':
				cd = optarg;
				break;
			case 'n':
				nicelevel = parse_long(optarg);
				break;
			case 'l':
			case 'L':
				lock      = optarg;
				lockdelay = opt == 'l';
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
			case '?':
				fprintf(stderr, "usage\n");
				return 1;
		}
	}
	argv += optind, argc -= optind;

	if (argc == 0) {
		fprintf(stderr, "command required\n");
		return 1;
	}

	if (ssid)
		setsid();

	if (uid) {
		setgroups(gid_len, gid);
		setgid(gid[0]);
		setuid(uid);
		// $EUID
	}
	if (root) {
		if (chroot(root) == -1)
			print_error("unable to change root directory: %s\n");

		chdir("/");
	}
	if (cd) {
		chdir(cd);
	}
	if (nicelevel != 0) {
		if (nice(nicelevel) == -1)
			print_error("unable to set nice level: %s\n");
	}

	if (lock)
		slock(lock, lockdelay);

	if (closestd[0] && close(0) == -1)
		print_error("unable to close stdin: %s\n");
	if (closestd[1] && close(1) == -1)
		print_error("unable to close stdout: %s\n");
	if (closestd[2] && close(2) == -1)
		print_error("unable to close stderr: %s\n");

	if (limitd >= -1) {
#ifdef RLIMIT_DATA
		limit(RLIMIT_DATA, limitd);
#else
		fprintf(stderr, "system does not support RLIMIT_DATA\n");
#endif
	}
	if (limits >= -1) {
#ifdef RLIMIT_STACK
		limit(RLIMIT_STACK, limits);
#else
		fprintf(stderr, "system does not support RLIMIT_STACK\n");
#endif
	}
	if (limitl >= -1) {
#ifdef RLIMIT_MEMLOCK
		limit(RLIMIT_MEMLOCK, limitl);
#else
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
		fprintf(stderr, "system does neither support RLIMIT_VMEM nor RLIMIT_AS\n");
#	endif
#endif
	}
	if (limito >= -1) {
#if defined(RLIMIT_NOFILE)
		limit(RLIMIT_NOFILE, limito);
#elif defined(RLIMIT_OFILE)
		limit(RLIMIT_OFILE, limito);
#else
		fprintf(stderr, "system does neither support RLIMIT_NOFILE nor RLIMIT_OFILE\n");
#endif
	}
	if (limitp >= -1) {
#ifdef RLIMIT_NPROC
		limit(RLIMIT_NPROC, limitp);
#else
		fprintf(stderr, "system does not support RLIMIT_NPROC\n");
#endif
	}
	if (limitf >= -1) {
#ifdef RLIMIT_FSIZE
		limit(RLIMIT_FSIZE, limitf);
#else
		fprintf(stderr, "system does not support RLIMIT_FSIZE\n");
#endif
	}
	if (limitc >= -1) {
#ifdef RLIMIT_CORE
		limit(RLIMIT_CORE, limitc);
#else
		fprintf(stderr, "system does not support RLIMIT_CORE\n");
#endif
	}
	if (limitr >= -1) {
#ifdef RLIMIT_RSS
		limit(RLIMIT_RSS, limitr);
#else
		fprintf(stderr, "system does not support RLIMIT_RSS\n");
#endif
	}
	if (limitt >= -1) {
#ifdef RLIMIT_CPU
		limit(RLIMIT_CPU, limitt);
#else
		fprintf(stderr, "system does not support RLIMIT_CPU\n");
#endif
	}
	const char* exec = argv[0];
	if (arg0)
		argv[0] = arg0;

	execvp(exec, argv);
	print_error("cannot execute: %s\n");
}
