// daemon manager

#include "config.h"
#include "service.h"
#include "util.h"

#include <getopt.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#define SV_DEPENDS_MAX_STR  static_stringify(SV_DEPENDS_MAX)
#define MAX_SERVICE_STR     static_stringify(SV_SERVICE_MAX)
#define SV_STOP_TIMEOUT_STR static_stringify(SV_STOP_TIMEOUT)


static const char HELP_MESSAGE[] =
    "Usage:\n"
    "  %s [options] <runlevel>\n"
    "\n"
    "Options:\n"
    "  -h, --help ........ prints this and exits\n"
    "  -v, --verbose ..... print more info\n"
    "  -V, --version ..... prints current version and exits\n"
    "  -f, --force ....... forces socket\n"
    "\n";

static const char VERSION_MESSAGE[] =
    "FISS v" SV_VERSION "\n"
    "\n"
    "Features:\n"
    " service directory:      " SV_SERVICE_DIR "\n"
    " service control socket: " SV_CONTROL_SOCKET "\n"
    " max. services:          " MAX_SERVICE_STR "\n"
    " max. dependencies:      " SV_DEPENDS_MAX_STR "\n"
    " stop timeout:           " SV_STOP_TIMEOUT_STR "sec\n"
    "\n";

static const struct option long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ "verbose", no_argument, 0, 'v' },
	{ "version", no_argument, 0, 'V' },
	{ "force", no_argument, 0, 'f' },
	{ 0 }
};

static void signal_interrupt(int signum) {
	(void) signum;

	daemon_running = false;
}

int main(int argc, char** argv) {
	bool force_socket = false;

	int c;
	while ((c = getopt_long(argc, argv, ":hvVf", long_options, NULL)) > 0) {
		switch (c) {
			case 'h':
				printf(VERSION_MESSAGE, "<runlevel>");
				printf(HELP_MESSAGE, argv[0]);
				return 0;
			case 'v':
				verbose = true;
				break;
			case 'V':
				printf(VERSION_MESSAGE, "<runlevel>");
				return 0;
			case 'f':
				force_socket = true;
				break;
			case '?':
				if (optopt)
					fprintf(stderr, "error: invalid option -%c\n", optopt);
				else
					fprintf(stderr, "error: invalid option %s\n", argv[optind - 1]);
				return 1;
		}
	}

	argv += optind;
	argc -= optind;
	if (argc == 0) {
		fprintf(stderr, "error: missing <service-dir>\n");
		return 1;
	} else if (argc == 1) {
		fprintf(stderr, "error: missing <runlevel>\n");
		return 1;
	} else if (argc > 2) {
		fprintf(stderr, "error: too many arguments\n");
		return 1;
	}

	signal(SIGINT, signal_interrupt);
	signal(SIGCONT, signal_interrupt);

	return service_supervise(argv[0], argv[1], force_socket);
}
