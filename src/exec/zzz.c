#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define write_state(state_fd, ident)                                                \
	do {                                                                            \
		if (write((state_fd), (ident), sizeof(ident) - 1) == -1)                    \
			fprintf(stderr, "error writing to /sys/power/: %s\n", strerror(errno)); \
	} while (0)

typedef enum zzz_mode {
	ZZZ_NOOP,
	ZZZ_STANDBY,
	ZZZ_SUSPEND,
	ZZZ_HIBERNATE_PLATFORM,
	ZZZ_HIBERNATE_REBOOT,
	ZZZ_HIBERNATE_SUSPEND,
} zzz_mode_t;

int main(int argc, char** argv) {
	int        opt;
	zzz_mode_t mode = ZZZ_SUSPEND;

	while ((opt = getopt(argc, argv, "nSzZRH")) != -1) {
		switch (opt) {
			case 'n':
				mode = ZZZ_NOOP;
				break;
			case 'S':
				mode = ZZZ_STANDBY;
				break;
			case 'z':
				mode = ZZZ_SUSPEND;
				break;
			case 'Z':
				mode = ZZZ_HIBERNATE_PLATFORM;
				break;
			case 'R':
				mode = ZZZ_HIBERNATE_REBOOT;
				break;
			case 'H':
				mode = ZZZ_HIBERNATE_SUSPEND;
				break;
			default:
				printf("zzz [-n] [-S] [-z] [-Z] [-R] [-H]\n");
				return 1;
		}
	}

	argc -= optind, argv += optind;

	int sys_state, sys_disk;
	if ((sys_state = open("/sys/power/state", O_WRONLY | O_TRUNC)) == -1) {
		fprintf(stderr, "cannot open /sys/power/state: %s\n", strerror(errno));
		return 1;
	}
	if ((sys_disk = open("/sys/power/disk", O_WRONLY | O_TRUNC)) == -1) {
		fprintf(stderr, "cannot open /sys/power/disk: %s\n", strerror(errno));
		return 1;
	}

	printf("Zzzz...\n");

	switch (mode) {
		case ZZZ_STANDBY:
			write_state(sys_state, "freeze");
			break;

		case ZZZ_SUSPEND:
			write_state(sys_state, "mem");
			break;

		case ZZZ_HIBERNATE_PLATFORM:
			write_state(sys_disk, "platform");
			write_state(sys_state, "disk");
			break;
		case ZZZ_HIBERNATE_REBOOT:
			write_state(sys_disk, "reboot");
			write_state(sys_state, "disk");
			break;
		case ZZZ_HIBERNATE_SUSPEND:
			write_state(sys_disk, "suspend");
			write_state(sys_state, "disk");
			break;

		case ZZZ_NOOP:
			sleep(5);
			break;
	}

	close(sys_state);
	close(sys_disk);

	printf("Yawn!\n");
}