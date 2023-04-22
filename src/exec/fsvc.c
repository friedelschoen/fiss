#include "config.h"
#include "service.h"
#include "signame.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static const char HELP_MESSAGE[] =
    "Usage:\n"
    "  %s <command> [-cfopqvV] [-r ..] [-s ..] [service]\n"
    "\n"
    "Check the manual (fsvc 8) for more information.\n";

static const char VERSION_MESSAGE[] =
    "FISS v" SV_VERSION "\n";


void print_status(service_t* s, char* state, size_t size) {
	switch (s->state) {
		case STATE_INACTIVE:
			strcpy(state, "inactive");
			break;
		case STATE_STARTING:
			strcpy(state, "starting");
			break;
		case STATE_ACTIVE_PID:
			snprintf(state, size, "active (pid) as %d", s->pid);
			break;
		case STATE_ACTIVE_BACKGROUND:
			strcpy(state, "active (background)");
			break;
		case STATE_ACTIVE_DUMMY:
			strcpy(state, "active (dummy)");
			break;
		case STATE_ACTIVE_FOREGROUND:
			snprintf(state, size, "active as %d", s->pid);
			break;
		case STATE_FINISHING:
			strcpy(state, "finishing");
			break;
		case STATE_STOPPING:
			strcpy(state, "stopping");
			break;
		case STATE_DEAD:
			strcpy(state, "dead");
			break;
	}
	time_t      diff      = time(NULL) - s->status_change;
	const char* diff_unit = "sec";
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
	int len = strlen(state);
	snprintf(state + len, size - len, " since %lu%s", diff, diff_unit);
}

void print_service(service_t* s, service_t* log) {
	if (s->is_log_service)
		return;
	char state[100];
	print_status(s, state, sizeof(state));

	printf("- %s (%s)\n", s->name, state);
	printf("  [ %c ] restart on exit\n", s->restart_file || s->restart_manual ? 'x' : ' ');
	printf("  [%3d] last return code (%s)\n", s->return_code, s->last_exit == EXIT_SIGNALED ? "signaled" : "exited");
	if (s->log_service) {
		print_status(log, state, sizeof(state));
		printf("        logging: %s\n", state);
	}
	printf("\n");
}

void print_service_short(service_t* s, service_t* log) {
	bool active = s->state == STATE_ACTIVE_BACKGROUND ||
	              s->state == STATE_ACTIVE_DUMMY ||
	              s->state == STATE_ACTIVE_FOREGROUND ||
	              s->state == STATE_ACTIVE_PID;

	bool restart = s->restart_file == S_ONCE ||
	               s->restart_file == S_RESTART ||
	               s->restart_manual == S_ONCE ||
	               s->restart_manual == S_RESTART;

	bool wants_other = active != restart;

	if (s->state == STATE_STARTING)
		printf("[    {+}]");
	else if (s->state == STATE_FINISHING || s->state == STATE_STOPPING)
		printf("[{-}     ]");
	else if (active)
		printf("[  %s + ]", wants_other ? "<-" : "  ");
	else
		printf("[ - %s  ]", wants_other ? "->" : "  ");

	printf(" %s", s->name);
	if (s->state == STATE_ACTIVE_PID || s->state == STATE_ACTIVE_FOREGROUND)
		printf(" (pid: %d)", s->pid);
	else if (s->last_exit != EXIT_SIGNALED)
		printf(" (last exit: SIG%s)", strsignal(s->return_code));
	else if (s->last_exit != EXIT_NORMAL)
		printf(" (last exit: %d)", s->return_code);
	printf("\n");

	if (log) {
		printf("log: ");
		print_service_short(log, NULL);
	}
}

static const struct option long_options[] = {
	{ "verbose", no_argument, 0, 'v' },
	{ "version", no_argument, 0, 'V' },
	{ "runlevel", no_argument, 0, 'r' },
	{ "service-dir", no_argument, 0, 's' },
	{ "pin", no_argument, 0, 'p' },
	{ "once", no_argument, 0, 'o' },
	{ "check", no_argument, 0, 'c' },
	{ "reset", no_argument, 0, 'f' },
	{ "short", no_argument, 0, 'q' },
	{ 0 }
};

int main(int argc, char** argv) {
	strcpy(runlevel, getenv(SV_RUNLEVEL_ENV) ?: SV_RUNLEVEL);
	service_dir = SV_SERVICE_DIR;

	bool check = false,
	     pin   = false,
	     once  = false,
	     reset = false,

	     short_ = false;

	int c;
	while ((c = getopt_long(argc, argv, ":Vvqs:r:pocf", long_options, NULL)) > 0) {
		switch (c) {
			case 'r':
				strcpy(runlevel, optarg);
				break;
			case 's':
				service_dir = optarg;
				break;
			case 'q':
				short_ = true;
				break;
			case 'v':
				verbose = true;
				break;
			case 'V':
				printf(VERSION_MESSAGE);
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
				fprintf(stderr, "%s", HELP_MESSAGE);
				return 1;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 1) {
		printf("fsvc [options] <command> [service]\n");
		return 1;
	}

	const char* command_str = argv[0];
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

	if (check)
		printf("warn: --check specified but not used\n");
	if (pin)
		printf("warn: --pin specified but not used\n");
	if (once)
		printf("warn: --once specified but not used\n");
	if (reset)
		printf("warn: --reset specified but not used\n");


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
			if (short_)
				print_service_short(&response[i], log);
			else
				print_service(&response[i], log);
		}
	}
}