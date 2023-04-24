#include "wtmp.h"

#include <err.h>
#include <stdbool.h>
#include <string.h>
#include <sys/reboot.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
	bool do_sync  = true,
	     do_force = false,
	     do_wtmp  = true,
	     noop     = false;
	int opt;

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
				errx(1, "Usage: reboot [-n] [-f] [-d] [-w] [-B]");
		}

	if (do_wtmp)
		write_wtmp(0);

	if (do_sync)
		sync();

	if (!noop) {
		if (do_force)
			reboot(RB_AUTOBOOT);
		else
			execl("/sbin/finit", "init", "6", NULL);
		err(1, "reboot failed");
	}

	return 0;
}
