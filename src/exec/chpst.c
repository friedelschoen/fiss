#include "user_group.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>

static long parse_long(const char* str) {
	char* end;
	long  l = strtol(str, &end, 10);
	if (*end != '\0') {
		fprintf(stderr, "error: invalid limit '%s'\n", optarg);
		exit(1);
	}
	return l;
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
	long  nicelevel = 0;
	bool  lockdelay, ssid = false;
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
			case 'e':    // ignored
				fprintf(stderr, "`envdir` is ignored\n");
				break;
			case 'd':
			case 'o':
			case 'p':
			case 'f':
			case 'c':
			case 'r':
			case 't':
			case 'm':    // ignored
				fprintf(stderr, "limits are ignored\n");
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

	if (lock)
		slock(lock, lockdelay);

	if (closestd[0] && close(0) == -1)
		print_error("unable to close stdin: %s\n");
	if (closestd[1] && close(1) == -1)
		print_error("unable to close stdout: %s\n");
	if (closestd[2] && close(2) == -1)
		print_error("unable to close stderr: %s\n");

	const char* exec = argv[0];
	if (arg0)
		argv[0] = arg0;

	execvp(exec, argv);
	print_error("cannot execute: %s\n");
}
