#include "config.h"
#include "service.h"

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>


static const char HELP_MESSAGE[] =
    "Usage:\n"
    "  %s [options] <runlevel>\n"
    "\n"
    "Options:\n"
    "  -h, --help ............... prints this and exits\n"
    "  -i, --as-init ............ execute start/stop script\n"
    "  -o, --stdout ............. print service stdout/stderr in console\n"
    "  -s, --service-dir <path> . using service-dir (default: " SV_SERVICE_DIR ")\n"
    "  -v, --verbose ............ print more info\n"
    "  -V, --version ............ prints current version and exits\n"
    "\n";

static const char VERSION_MESSAGE[] =
    "FISS v" SV_VERSION "\n";

void print_status(service_t* s) {
	const char* state;
	switch (s->state) {
		case STATE_INACTIVE:
			state = "inactive";
			break;
		case STATE_STARTING:
			state = "starting";
			break;
		case STATE_ACTIVE_PID:
			state = "active (pid)";
			break;
		case STATE_ACTIVE_BACKGROUND:
			state = "active (background)";
			break;
		case STATE_ACTIVE_DUMMY:
			state = "active (dummy)";
			break;
		case STATE_ACTIVE_FOREGROUND:
			state = "active";
			break;
		case STATE_FINISHING:
			state = "finishing";
			break;
		case STATE_STOPPING:
			state = "stopping";
			break;
		case STATE_DEAD:
			state = "dead";
			break;
	}
	time_t diff      = time(NULL) - s->status_change;
	string diff_unit = "sec";
	if (diff >= 60) {
		diff /= 60;
		diff_unit = "min";
	}
	if (diff >= 60) {
		diff /= 60;
		diff_unit = "hours";
	}
	if (diff >= 24) {
		diff /= 24;
		diff_unit = "days";
	}
	printf("%s since %lu%s", state, diff, diff_unit);
}

void print_service(service_t* s, service_t* log) {
	printf("- %s (", s->name);
	print_status(s);
	printf(")\n");
	printf("  [ %c ] restart on exit\n", s->restart ? 'x' : ' ');
	printf("  [%3d] last return code (%s)\n", s->return_code, s->last_exit == EXIT_SIGNALED ? "signaled" : "exited");
	if (s->log_service) {
		printf("        logging: ");
		print_status(log);
		printf("\n");
	}
	printf("\n");
}

static const struct option long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ "verbose", no_argument, 0, 'v' },
	{ "version", no_argument, 0, 'V' },
	{ "runlevel", no_argument, 0, 'r' },
	{ "service-dir", no_argument, 0, 's' },
	{ "pin", no_argument, 0, 'n' },
	{ "now", no_argument, 0, 'p' },
	{ 0 }
};

int main(int argc, char** argv) {
	runlevel    = getenv(SV_RUNLEVEL_ENV) ?: SV_RUNLEVEL;
	service_dir = SV_SERVICE_DIR;

	int c;
	while ((c = getopt_long(argc, argv, ":hVvnps:r:", long_options, NULL)) > 0) {
		switch (c) {
			case 'r':
				runlevel = optarg;
				break;
			case 's':
				service_dir = optarg;
				break;
			case 'v':
				verbose = true;
				break;
			case 'V':
				printf(VERSION_MESSAGE);
				return 0;
			case 'h':
				printf(HELP_MESSAGE, argv[0]);
				return 0;
			case '?':
				if (optopt)
					fprintf(stderr, "error: invalid option -%c\n", optopt);
				else
					fprintf(stderr, "error: invalid option %s\n", argv[optind - 1]);
				return 1;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 1) {
		printf("%s [options] <command> [service]\n", argv[0]);
		return 1;
	}

	char*  command = argv[0];
	string service = argc >= 2 ? argv[1] : "";
	int    extra   = argc >= 3 ? atoi(argv[2]) : 0;

	service_t response[50];
	int       res = 0;

	if (streq(command, "check")) {
		service_t s;
		int       rc;
		if ((rc = service_command(S_STATUS, 0, service, &s, 1)) != 1) {
			printf("error: %s (errno: %d)\n", command_error[-res], -res);
			return 1;
		}
		return s.state == STATE_ACTIVE_PID || s.state == STATE_ACTIVE_DUMMY || s.state == STATE_ACTIVE_FOREGROUND || s.state == STATE_ACTIVE_BACKGROUND;

	} else {
		char cmd_abbr;
		if ((cmd_abbr = service_get_command(command)) == 0)
			res = -EBADCMD;
		else
			res = service_command(cmd_abbr, extra, service, response, 50);

		if (res < 0) {
			printf("error: %s (errno: %d)\n", command_error[-res], -res);
			return 1;
		}

		for (int i = 0; i < res; i++) {
			service_t* log = NULL;
			if (response[i].log_service) {
				for (int j = 0; j < res; j++) {
					if (strncmp(response[i].name, response[j].name, strlen(response[i].name)) == 0) {
						log = &response[j];
						break;
					}
				}
			}
			print_service(&response[i], log);
		}
	}
}
