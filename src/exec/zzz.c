#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
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

	printf("Yawn!\n");
}