// +objects: message.o util.o supervise.o service.o start.o stop.o
// +objects: register.o handle_exit.o handle_command.o
// +objects: encode.o parse.o dependency.o pattern.o status.o

#include "config.h"
#include "message.h"
#include "service.h"
#include "util.h"

#include <getopt.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>


static const struct option long_options[] = {
	{ "version", no_argument, 0, 'V' },
	{ "once", no_argument, 0, 'o' },
	{ 0 }
};

static void signal_interrupt(int signum) {
	(void) signum;

	daemon_running = false;
}

int main(int argc, char** argv) {
	int  c;
	bool once = false;
	while ((c = getopt_long(argc, argv, ":Vo", long_options, NULL)) > 0) {
		switch (c) {
			case 'V':
				print_version_exit();
				break;
			case 'o':
				once = true;
				break;
			default:
			case '?':
				if (optopt)
					fprintf(stderr, "error: invalid option -%c\n", optopt);
				else
					fprintf(stderr, "error: invalid option %s\n", argv[optind - 1]);
				print_usage_exit(PROG_FSVS, 1);
		}
	}

	argv += optind;
	argc -= optind;
	if (argc == 0) {
		fprintf(stderr, "error: missing <service-dir>\n");
		print_usage_exit(PROG_FSVS, 1);
	} else if (argc == 1) {
		fprintf(stderr, "error: missing <runlevel>\n");
		print_usage_exit(PROG_FSVS, 1);
	} else if (argc > 2) {
		fprintf(stderr, "error: too many arguments\n");
		print_usage_exit(PROG_FSVS, 1);
	}

	struct sigaction sa = { 0 };
	sa.sa_handler       = signal_interrupt;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	return service_supervise(argv[0], argv[1], once);
}
