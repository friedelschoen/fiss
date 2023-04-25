#include "config.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>


int main(int argc, char** argv) {
	int sys_state, sys_disk, opt;

	const char *new_state = "suspend",
	           *new_disk  = NULL;

	while ((opt = getopt(argc, argv, "nSzZRH")) != -1) {
		switch (opt) {
			case 'n':
				new_state = NULL;
				break;
			case 'S':
				new_state = "freeze";
				break;
			case 'z':
				new_state = "mem";
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

	printf("Zzzz...\n");

	struct stat st;

	if (stat(SV_SUSPEND_EXEC, &st) == 0 && st.st_mode & S_IXUSR) {
		pid_t pid;
		if ((pid = fork()) == -1) {
			fprintf(stderr, "failed to fork for " SV_SUSPEND_EXEC ": %s\n", strerror(errno));
			return 1;
		} else if (pid == 0) {    // child
			execl(SV_SUSPEND_EXEC, SV_SUSPEND_EXEC, NULL);
			fprintf(stderr, "failed to execute " SV_SUSPEND_EXEC ": %s\n", strerror(errno));
			_exit(1);
		}

		wait(NULL);
	}

	if (new_disk) {
		if ((sys_disk = open("/sys/power/disk", O_WRONLY | O_TRUNC)) == -1) {
			fprintf(stderr, "cannot open /sys/power/disk: %s\n", strerror(errno));
			return 1;
		}
		if (write(sys_disk, new_disk, strlen(new_disk)) == -1)
			fprintf(stderr, "error writing to /sys/power/disk: %s\n", strerror(errno));

		close(sys_disk);
	}

	if (new_state) {
		if ((sys_state = open("/sys/power/state", O_WRONLY | O_TRUNC)) == -1) {
			fprintf(stderr, "cannot open /sys/power/state: %s\n", strerror(errno));
			return 1;
		}
		if (write(sys_state, new_state, strlen(new_state)) == -1)
			fprintf(stderr, "error writing to /sys/power/state: %s\n", strerror(errno));

		close(sys_state);
	} else {
		sleep(5);
	}

	if (stat(SV_RESUME_EXEC, &st) == 0 && st.st_mode & S_IXUSR) {
		pid_t pid;
		if ((pid = fork()) == -1) {
			fprintf(stderr, "failed to fork for " SV_RESUME_EXEC ": %s\n", strerror(errno));
			return 1;
		} else if (pid == 0) {    // child
			execl(SV_RESUME_EXEC, SV_RESUME_EXEC, NULL);
			fprintf(stderr, "failed to execute " SV_RESUME_EXEC ": %s\n", strerror(errno));
			_exit(1);
		}

		wait(NULL);
	}

	printf("Yawn!\n");
}