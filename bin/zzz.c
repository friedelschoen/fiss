#include "config.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>


const char* current_prog(void) {
	return "zzz";
}

static int open_write(const char* path, const char* string) {
	int fd;

	if ((fd = open(path, O_WRONLY | O_TRUNC)) == -1) {
		print_errno("cannot open %s: %s\n", path);
		return -1;
	}
	if (write(fd, string, strlen(string)) == -1) {
		print_errno("error writing to %s: %s\n", path);
		close(fd);
		return -1;
	}
	return close(fd);
}


int main(int argc, char** argv) {
	int         opt;
	pid_t       pid;
	struct stat st;

	const char *new_state = NULL,
	           *new_disk  = NULL;

	if (streq(argv[0], "zzz")) {
		new_state = "mem";
		new_disk  = NULL;
	} else if (streq(argv[0], "ZZZ")) {
		new_state = "disk";
		new_disk  = "platform";
	} else {
		fprintf(stderr, "error: program-name `%s` invalid\n", argv[0]);
		return 1;
	}

	struct option long_options[] = {
		{ "noop", no_argument, 0, 'n' },
		{ "freeze", no_argument, 0, 'S' },
		{ "suspend", no_argument, 0, 'z' },
		{ "hibernate", no_argument, 0, 'Z' },
		{ "reboot", no_argument, 0, 'R' },
		{ "hybrid", no_argument, 0, 'H' },
		{ 0 },
	};

	while ((opt = getopt_long(argc, argv, "nSzZRH", long_options, NULL)) != -1) {
		switch (opt) {
			case 'n':
				new_state = NULL;
				new_disk  = NULL;
				break;
			case 's':
				new_state = "suspend";
				new_disk  = NULL;
				break;
			case 'S':
				new_state = "freeze";
				new_disk  = NULL;
				break;
			case 'z':
				new_state = "mem";
				new_disk  = NULL;
				break;
			case 'Z':
				new_state = "disk";
				new_disk  = "platform";
				break;
			case 'R':
				new_state = "disk";
				new_disk  = "reboot";
				break;
			case 'H':
				new_state = "disk";
				new_disk  = "suspend";
				break;
			default:
				printf("zzz [-n] [-S] [-z] [-Z] [-R] [-H]\n");
				return 1;
		}
	}

	argc -= optind, argv += optind;


	if (stat(SV_SUSPEND_EXEC, &st) == 0 && st.st_mode & S_IXUSR) {
		if ((pid = fork()) == -1) {
			print_errno("failed to fork for " SV_SUSPEND_EXEC ": %s\n");
			return 1;
		} else if (pid == 0) {    // child
			execl(SV_SUSPEND_EXEC, SV_SUSPEND_EXEC, NULL);
			print_errno("failed to execute " SV_SUSPEND_EXEC ": %s\n");
			_exit(1);
		}

		wait(NULL);
	}

	if (new_disk) {
		open_write("/sys/power/disk", new_disk);
	}

	if (new_state) {
		open_write("/sys/power/state", new_state);
	} else {
		sleep(5);
	}

	if (stat(SV_RESUME_EXEC, &st) == 0 && st.st_mode & S_IXUSR) {
		if ((pid = fork()) == -1) {
			print_errno("failed to fork for " SV_RESUME_EXEC ": %s\n");
			return 1;
		} else if (pid == 0) {    // child
			execl(SV_RESUME_EXEC, SV_RESUME_EXEC, NULL);
			print_errno("failed to execute " SV_RESUME_EXEC ": %s\n");
			_exit(1);
		}

		wait(NULL);
	}
}
