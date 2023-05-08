// daemon manager

#include "config.h"
#include "message.h"
#include "service.h"
#include "util.h"

#include <getopt.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#define SV_DEPENDS_MAX_STR  static_stringify(SV_DEPENDS_MAX)
#define MAX_SERVICE_STR     static_stringify(SV_SERVICE_MAX)
#define SV_STOP_TIMEOUT_STR static_stringify(SV_STOP_TIMEOUT)


static const struct option long_options[] = {
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
	while ((c = getopt_long(argc, argv, ":vVf", long_options, NULL)) > 0) {
		switch (c) {
			case 'v':
				verbose = true;
				break;
			case 'V':
				print_version_exit();
			case 'f':
				force_socket = true;
				break;
			case '?':
				if (optopt)
					fprintf(stderr, "error: invalid option -%c\n", optopt);
				else
					fprintf(stderr, "error: invalid option %s\n", argv[optind - 1]);
				print_usage_exit(PROG_FSVC, 1);
		}
	}

	argv += optind;
	argc -= optind;
	if (argc == 0) {
		fprintf(stderr, "error: missing <service-dir>\n");
		print_usage_exit(PROG_FSVC, 1);
	} else if (argc == 1) {
		fprintf(stderr, "error: missing <runlevel>\n");
		print_usage_exit(PROG_FSVC, 1);
	} else if (argc > 2) {
		fprintf(stderr, "error: too many arguments\n");
		print_usage_exit(PROG_FSVC, 1);
	}

	signal(SIGINT, signal_interrupt);
	signal(SIGCONT, signal_interrupt);

	return service_supervise(argv[0], argv[1], force_socket);
}
