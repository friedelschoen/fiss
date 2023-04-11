#include "config.h"
#include "service.h"
#include "signame.h"

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
	printf("  [ %c ] restart on exit\n", s->restart_file || s->restart_manual ? 'x' : ' ');
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
	{ "pin", no_argument, 0, 'p' },
	{ "once", no_argument, 0, 'o' },
	{ "check", no_argument, 0, 'c' },
	{ "reset", no_argument, 0, 'f' },
	{ 0 }
};

int main(int argc, char** argv) {
	strcpy(runlevel, getenv(SV_RUNLEVEL_ENV) ?: SV_RUNLEVEL);
	service_dir = SV_SERVICE_DIR;

	bool check = false,
	     pin   = false,
	     once  = false,
	     reset = false;
	int c;
	while ((c = getopt_long(argc, argv, ":hVvs:r:pocf", long_options, NULL)) > 0) {
		switch (c) {
			case 'r':
				strcpy(runlevel, optarg);
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
			case 'p':
				pin = true;
				break;
			case 'o':
				once = true;
				break;
			case 'c':
				check = true;
				break;
			case 'f':
				reset = true;
				break;
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
		printf("fsvc [options] <command> [service]\n");
		return 1;
	}

	string command_str = argv[0];
	argv++;
	argc--;

	const char* service = NULL;
	char        command, extra = 0;

	if (streq(command_str, "up") || streq(command_str, "start") || streq(command_str, "down") || streq(command_str, "stop")) {
		if (argc == 0) {
			printf("service omitted\n");
			return 1;
		} else if (argc > 1) {
			printf("redundant argument '%s'\n", argv[2]);
			return 1;
		}

		command = streq(command_str, "down") || streq(command_str, "stop") ? S_STOP : S_START;
		extra   = pin;
		service = argv[0];
		pin     = false;
	} else if (streq(command_str, "send") || streq(command_str, "kill")) {
		if (argc == 0) {
			printf("service omitted\n");
			return 1;
		} else if (argc == 1) {
			printf("signal omitted\n");
			return 1;
		} else if (argc > 2) {
			printf("redundant argument '%s'\n", argv[2]);
			return 1;
		}

		char* endptr;
		extra = strtol(argv[1], &endptr, 10);
		if (endptr == argv[1]) {
			if ((extra = signame(argv[1])) == 0) {
				printf("unknown signalname\n");
				return 1;
			}
		} else if (endptr != strchr(argv[1], '\0')) {
			printf("malformatted signal\n");
			return 1;
		}

		command = S_SEND;
		service = argv[0];
	} else if (streq(command_str, "enable") || streq(command_str, "disable")) {
		if (argc == 0) {
			printf("service omitted\n");
			return 1;
		} else if (argc > 1) {
			printf("redundant argument '%s'\n", argv[2]);
			return 1;
		}

		command = streq(command_str, "enable") ? S_ENABLE : S_DISABLE;
		extra   = once;
		once    = false;
		service = argv[0];
	} else if (streq(command_str, "status")) {
		if (argc == 1) {
			service = argv[0];
		} else if (argc > 1) {
			printf("redundant argument '%s'\n", argv[2]);
			return 1;
		}

		command = S_STATUS;
		extra   = check;
		check   = false;
	} else if (streq(command_str, "pause") || streq(command_str, "resume")) {
		if (argc == 0) {
			printf("service omitted\n");
			return 1;
		} else if (argc > 1) {
			printf("redundant argument '%s'\n", argv[2]);
			return 1;
		}

		command = streq(command_str, "pause") ? S_PAUSE : S_RESUME;
		service = argv[0];
	} else if (streq(command_str, "switch")) {
		if (argc == 0) {
			printf("runlevel omitted\n");
			return 1;
		} else if (argc > 1) {
			printf("redundant argument '%s'\n", argv[2]);
			return 1;
		}

		command = S_SWITCH;
		extra   = reset;
		reset   = false;
		service = argv[0];
	} else {
		printf("unknown command '%s'\n", command_str);
		return 1;
	}

	service_t response[50];
	int       rc;

	if (check) {
		service_t s;
		if ((rc = service_command(command, extra, service, &s, 1)) != 1) {
			printf("error: %s (errno: %d)\n", command_error[-rc], -rc);
			return 1;
		}
		return s.state == STATE_ACTIVE_PID || s.state == STATE_ACTIVE_DUMMY || s.state == STATE_ACTIVE_FOREGROUND || s.state == STATE_ACTIVE_BACKGROUND;
	} else {
		rc = service_command(command, extra, service, response, 50);

		if (rc < 0) {
			printf("error: %s (errno: %d)\n", command_error[-rc], -rc);
			return 1;
		}

		for (int i = 0; i < rc; i++) {
			service_t* log = NULL;
			if (response[i].log_service) {
				for (int j = 0; j < rc; j++) {
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