// +objects: wtmp.o util.o

#include "util.h"
#include "wtmp.h"

#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <sys/reboot.h>
#include <unistd.h>


const char* current_prog(void) {
	return "halt";
}

int main(int argc, char* argv[]) {
	bool do_sync  = true,
	     do_force = false,
	     do_wtmp  = true,
	     noop     = false;
	int         opt;
	int         rebootnum;
	const char* initarg;

	char* prog = progname(argv[0]);

	if (streq(prog, "halt")) {
		rebootnum = RB_HALT_SYSTEM;
		initarg   = "0";
	} else if (streq(prog, "poweroff")) {
		rebootnum = RB_POWER_OFF;
		initarg   = "0";
	} else if (streq(prog, "reboot")) {
		rebootnum = RB_AUTOBOOT;
		initarg   = "6";
	} else {
		fprintf(stderr, "invalid mode: %s\n", prog);
		return 1;
	}

	while ((opt = getopt(argc, argv, "dfhinwB")) != -1)
		switch (opt) {
			case 'n':
				do_sync = 0;
				break;
			case 'w':
				noop    = 1;
				do_sync = 0;
				break;
			case 'd':
				do_wtmp = 0;
				break;
			case 'h':
			case 'i':
				/* silently ignored.  */
				break;
			case 'f':
				do_force = 1;
				break;
			case 'B':
				write_wtmp(1);
				return 0;
			default:
				fprintf(stderr, "Usage: %s [-n] [-f] [-d] [-w] [-B]", prog);
				return 1;
		}

	if (do_wtmp)
		write_wtmp(0);

	if (do_sync)
		sync();

	if (!noop) {
		if (do_force) {
			reboot(rebootnum);
		} else {
			execl("/sbin/init", "init", initarg, NULL);
		}
		print_errno("reboot failed: %s\n");
	}

	return 0;
}
