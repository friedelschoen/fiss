#include "config.h"
#include "service.h"
#include "signame.h"

#include <ctype.h>
#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static const char HELP_MESSAGE[] =
    "Usage:\n"
    "  %s <command> [-cfopqvV] [-r ..] [-s ..] [service]\n"
    "  /etc/init.d/<service> [-cfopqvV] [-r ..] [-s ..] <command>\n"
    "\n"
    "Check the manual (fsvc 8) for more information.\n";

static const char VERSION_MESSAGE[] =
    "FISS v" SV_VERSION "\n";


void print_status(service_t* s, char* state, size_t size) {
	switch (s->state) {
		case STATE_SETUP:
			strncpy(state, "setup", size);
			break;
		case STATE_INACTIVE:
			strncpy(state, "inactive", size);
			break;
		case STATE_STARTING:
			strncpy(state, "starting", size);
			break;
		case STATE_ACTIVE_PID:
			snprintf(state, size, "active (pid) as %d", s->pid);
			break;
		case STATE_ACTIVE_BACKGROUND:
			strncpy(state, "active (background)", size);
			break;
		case STATE_ACTIVE_DUMMY:
			strncpy(state, "active (dummy)", size);
			break;
		case STATE_ACTIVE_FOREGROUND:
			snprintf(state, size, "active as %d", s->pid);
			break;
		case STATE_FINISHING:
			strncpy(state, "finishing", size);
			break;
		case STATE_STOPPING:
			strncpy(state, "stopping", size);
			break;
		case STATE_DEAD:
			strncpy(state, "dead", size);
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

void print_service(service_t* s) {
	char state[100];
	print_status(s, state, sizeof(state));

	printf("- %s (%s)\n", s->name, state);
	printf("  [ %c ] restart on exit\n", s->restart_file || s->restart_manual ? 'x' : ' ');
	printf("  [%3d] last return code (%s)\n", s->return_code, s->last_exit == EXIT_SIGNALED ? "signaled" : "exited");
	printf("\n");
}

static char* to_upper(char* str) {
	for (char* c = str; *c; c++) {
		if (islower(*c))
			*c += 'A' - 'a';
	}
	return str;
}

void print_service_short(service_t* s) {
	bool active = s->state == STATE_ACTIVE_BACKGROUND ||
	              s->state == STATE_ACTIVE_DUMMY ||
	              s->state == STATE_ACTIVE_FOREGROUND ||
	              s->state == STATE_ACTIVE_PID;

	bool wants_other = active != s->restart_final;

	if (s->state == STATE_SETUP || s->state == STATE_STARTING)
		printf("[    {+}]");
	else if (s->state == STATE_FINISHING || s->state == STATE_STOPPING)
		printf("[{-}     ]");
	else if (active)
		printf("[  %s + ]", wants_other ? "<-" : "  ");
	else
		printf("[ - %s  ]", wants_other ? "->" : "  ");

	printf(" %s", s->name);

	if (s->pid) {
		if (s->state == STATE_ACTIVE_PID || s->state == STATE_ACTIVE_FOREGROUND) {
			printf(" (pid: %d)", s->pid);
		} else if (s->last_exit == EXIT_SIGNALED) {
			printf(" (last exit: SIG%s)", sigabbr(s->return_code));
		} else if (s->last_exit == EXIT_NORMAL) {
			printf(" (last exit: %d)", s->return_code);
		}
	}
	printf("\n");
}

static const struct option long_options[] = {
	{ "verbose", no_argument, 0, 'v' },
	{ "version", no_argument, 0, 'V' },
	{ "runlevel", no_argument, 0, 'r' },
	{ "pin", no_argument, 0, 'p' },
	{ "once", no_argument, 0, 'o' },
	{ "check", no_argument, 0, 'c' },
	{ "reset", no_argument, 0, 'f' },
	{ "short", no_argument, 0, 'q' },
	{ 0 }
};

int main(int argc, char** argv) {
	strncpy(runlevel, getenv(SV_RUNLEVEL_DEFAULT_ENV) ? getenv(SV_RUNLEVEL_DEFAULT_ENV) : SV_RUNLEVEL_DEFAULT, SV_NAME_MAX);

	char* argexec = argv[0];

	bool check = false,
	     pin   = false,
	     once  = false,
	     reset = false,

	     short_ = false;

	int c;
	while ((c = getopt_long(argc, argv, ":Vvqr:pocf", long_options, NULL)) > 0) {
		switch (c) {
			case 'r':
				strncpy(runlevel, optarg, SV_NAME_MAX);
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
	argv++, argc--;

	const char* service;
	char        command, extra = 0;

	if (streq(service = basename(argexec), "fsvc")) {
		if (argc > 0) {
			service = argv[0];
			argv++, argc--;
		} else {
			service = NULL;
		}
	}

	if (streq(command_str, "up") || streq(command_str, "start") || streq(command_str, "down") || streq(command_str, "stop")) {
		if (!service) {
			printf("service omitted\n");
			return 1;
		}

		command = streq(command_str, "down") || streq(command_str, "stop") ? S_STOP : S_START;
		extra   = pin;
		pin     = false;
	} else if (streq(command_str, "send") || streq(command_str, "kill")) {
		if (!service) {
			printf("service omitted\n");
			return 1;
		}
		if (argc == 1) {
			printf("signal omitted\n");
			return 1;
		}
		if ((extra = signame(argv[1])) == -1) {
			printf("unknown signalname\n");
			return 1;
		}

		command = S_SEND;
	} else if (streq(command_str, "enable") || streq(command_str, "disable")) {
		if (!service) {
			printf("service omitted\n");
			return 1;
		}

		command = streq(command_str, "enable") ? S_ENABLE : S_DISABLE;
		extra   = once;
		once    = false;
	} else if (streq(command_str, "status")) {
		command = S_STATUS;
		extra   = check;
		check   = false;
	} else if (streq(command_str, "pause") || streq(command_str, "resume")) {
		if (!service) {
			printf("service omitted\n");
			return 1;
		}

		command = streq(command_str, "pause") ? S_PAUSE : S_RESUME;
	} else if (streq(command_str, "switch")) {
		if (!service) {
			printf("runlevel omitted\n");
			return 1;
		}

		command = S_SWITCH;
		extra   = reset;
		reset   = false;
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


	service_t response[SV_SOCKET_SERVICE_MAX];
	int       rc;

	if (check) {
		service_t s;
		if ((rc = service_command(command, extra, service, &s, 1)) != 1) {
			printf("error: %s (errno: %d)\n", command_error[-rc], -rc);
			return 1;
		}
		return s.state == STATE_ACTIVE_PID || s.state == STATE_ACTIVE_DUMMY || s.state == STATE_ACTIVE_FOREGROUND || s.state == STATE_ACTIVE_BACKGROUND;
	} else {
		rc = service_command(command, extra, service, response, SV_SOCKET_SERVICE_MAX);

		if (rc < 0) {
			printf("error: %s (errno: %d)\n", command_error[-rc], -rc);
			return 1;
		}

		for (int i = 0; i < rc; i++) {
			if (short_)
				print_service_short(&response[i]);
			else
				print_service(&response[i]);
		}
	}
}
