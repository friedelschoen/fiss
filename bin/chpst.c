#include "parse.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>


int main(int argc, char** argv) {
	int   opt, lockfd, lockflags, gid_len = 0;
	char *arg0 = NULL, *root = NULL, *cd = NULL, *lock = NULL, *exec = NULL;
	uid_t uid = 0;
	gid_t gid[61];
	long  nicelevel   = 0;
	bool  ssid        = false;
	bool  closestd[3] = { false, false, false };

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
			case 'e':
			case 'd':
			case 'o':
			case 'p':
			case 'f':
			case 'c':
			case 'r':
			case 't':
			case 'm':    // ignored
				fprintf(stderr, "warning: '-%c' are ignored\n", optopt);
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
			print_error("unable to change root directory: %s\n");

		// chdir to '/', otherwise the next command will complain 'directory not found'
		chdir("/");
	}

	if (cd) {
		chdir(cd);
	}

	if (nicelevel != 0) {
		if (nice(nicelevel) == -1)
			print_error("unable to set nice level: %s\n");
	}

	if (lock) {
		if ((lockfd = open(lock, O_WRONLY | O_APPEND)) == -1)
			print_error("unable to open lock: %s\n");

		if (flock(lockfd, lockflags) == -1)
			print_error("unable to lock: %s\n");
	}

	if (closestd[0] && close(0) == -1)
		print_error("unable to close stdin: %s\n");
	if (closestd[1] && close(1) == -1)
		print_error("unable to close stdout: %s\n");
	if (closestd[2] && close(2) == -1)
		print_error("unable to close stderr: %s\n");

	exec = argv[0];
	if (arg0)
		argv[0] = arg0;

	execvp(exec, argv);
	print_error("cannot execute: %s\n");
}
