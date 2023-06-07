#include "config.h"
#include "message.h"
#include "service.h"
#include "signame.h"
#include "util.h"

#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


struct ident {
	const char* name;
	const char* command;
	bool        runit;
};

static struct ident command_names[] = {
	{ "up", "u", true },                 // starts the services, pin as started
	{ "down", "d", true },               // stops the service, pin as stopped
	{ "xup", "U", true },                // stops the service, don't pin as stopped
	{ "xdown", "D", true },              // stops the service, don't pin as stopped
	{ "once", "o", true },               // same as xup
	{ "term", "t", true },               // same as down
	{ "kill", "k", true },               // sends kill, pin as stopped
	{ "pause", "p", true },              // pauses the service
	{ "cont", "c", true },               // resumes the service
	{ "reset", "r", true },              // resets the service
	{ "alarm", "a", true },              // sends alarm
	{ "hup", "h", true },                // sends hup
	{ "int", "i", true },                // sends interrupt
	{ "quit", "q", true },               // sends quit
	{ "1", "1", true },                  // sends usr1
	{ "2", "2", true },                  // sends usr2
	{ "exit", "x", true },               // does nothing
	{ "+up", "U0", false },              // starts the service, don't modify pin
	{ "+down", "D0", false },            // stops the service, don't modify pin
	{ "restart", "!D0U0", false },       // restarts the service, don't modify pin
	{ "start", "!u", true },             // start the service, pin as started, print status
	{ "stop", "!d", true },              // stop the service, pin as stopped, print status
	{ "status", "!", true },             // print status
	{ "check", "!", true },              // print status
	{ "enable", "!.e", false },          // enable service
	{ "disable", "!.d", false },         // disable service
	{ "enable-once", "!.e", false },     // enable service once
	{ "disable-once", "!.d", false },    // disable service once
	{ 0, 0, 0 }
};

static const struct option long_options[] = {
	{ "version", no_argument, NULL, 'V' },
	{ "wait", no_argument, NULL, 'w' },
	{ 0 }
};

static int check_service(int dir, char* runlevel) {
	int     fd;
	ssize_t size;

	if ((fd = openat(dir, "supervise/ok", O_WRONLY | O_NONBLOCK)) == -1)
		return -1;
	close(fd);

	if ((fd = openat(dir, "supervise/runlevel", O_RDONLY)) == -1)
		return -1;

	if ((size = read(fd, runlevel, NAME_MAX)) == -1) {
		close(fd);
		return -1;
	}
	runlevel[size] = '\0';
	close(fd);

	return 0;
}

static time_t get_mtime(int dir) {
	struct stat st;
	if (fstatat(dir, "supervise/status", &st, 0) == -1)
		return -1;
	return st.st_mtim.tv_sec;
}

static int handle_command(int dir, char command, const char* runlevel) {
	int fd;

	char up_file[SV_NAME_MAX]   = "up-";
	char once_file[SV_NAME_MAX] = "once-";

	strcat(up_file, runlevel);
	strcat(once_file, runlevel);

	switch (command) {
		case 'e':    // enable
			if ((fd = openat(dir, up_file, O_WRONLY | O_TRUNC | O_CREAT, 0644)) != -1)
				close(fd);
			break;
		case 'd':
			unlinkat(dir, up_file, 0);
			break;
		case 'E':    // enable
			if ((fd = openat(dir, once_file, O_WRONLY | O_TRUNC | O_CREAT, 0644)) != -1)
				close(fd);
			break;
		case 'D':
			unlinkat(dir, once_file, 0);
			break;
		default:
			return -1;
	}
	return 0;
}

static int send_command(int dir, const char* command, const char* runlevel) {
	int fd;
	if ((fd = openat(dir, "supervise/control", O_WRONLY | O_NONBLOCK)) == -1)
		return -1;

	for (const char* c = command; *c != '\0'; c++) {
		if (*c == '.') {
			c++;
			if (handle_command(dir, *c, runlevel) == -1)
				return -1;
		} else {
			if (write(fd, c, 1) == -1)
				break;
		}
	}
	close(fd);

	return 0;
}

int status(int dir) {
	int                   fd;
	time_t                timeval;
	const char*           timeunit = "sec";
	struct service_serial buffer;
	service_t             s;

	if ((fd = openat(dir, "supervise/status", O_RDONLY | O_NONBLOCK)) == -1)
		return -1;

	if (read(fd, &buffer, sizeof(buffer)) == -1) {
		close(fd);
		return -1;
	}

	close(fd);

	service_decode(&s, &buffer);

	timeval = time(NULL) - s.status_change;

	if (timeval >= 60) {
		timeval /= 60;
		timeunit = "min";
		if (timeval >= 60) {
			timeval /= 60;
			timeunit = "h";
			if (timeval >= 24) {
				timeval /= 24;
				timeunit = "d";
			}
		}
	}

	switch (s.state) {
		case STATE_SETUP:
			printf("setting up");
			break;
		case STATE_STARTING:
			printf("starting as %d", s.pid);
			break;
		case STATE_ACTIVE_FOREGROUND:
			printf("active as %d", s.pid);
			break;
		case STATE_ACTIVE_BACKGROUND:
		case STATE_ACTIVE_DUMMY:
			printf("active");
			break;
		case STATE_FINISHING:
			printf("finishing as %d", s.pid);
			break;
		case STATE_STOPPING:
			printf("stopping as %d", s.pid);
			break;
		case STATE_INACTIVE:
			printf("inactive");
			break;
		case STATE_DEAD:
			printf("dead");
			break;
	}

	if (s.paused)
		printf(" & paused");

	printf(" since %lu%s", timeval, timeunit);

	if (s.restart_manual == S_ONCE && s.restart_file != S_ONCE)
		printf(", manually started once");
	else if (s.restart_manual == S_RESTART && s.restart_file != S_RESTART)
		printf(", manually restart");
	else if (s.restart_manual == S_FORCE_DOWN && s.restart_file != S_DOWN)
		printf(", manually forced down");

	if (s.restart_file == S_ONCE)
		printf(", should started once");
	else if (s.restart_file == S_RESTART)
		printf(", should restart");

	if (s.is_dependency)
		printf(", started as dependency");

	if (s.return_code > 0 && s.last_exit == EXIT_NORMAL)
		printf(", exited with %d", s.return_code);

	if (s.return_code > 0 && s.last_exit == EXIT_SIGNALED)
		printf(", signaled with SIG%s", sigabbr(s.return_code));

	if (s.fail_count > 0)
		printf(", failed %d times", s.fail_count);

	printf("\n");

	return 0;
}

int main(int argc, char** argv) {
	int opt, dir,
	    timeout = SV_STATUS_WAIT;
	time_t mod, start;

	const char* command = NULL;
	char        runlevel[SV_NAME_MAX];

	while ((opt = getopt_long(argc, argv, ":Vw:", long_options, NULL)) != -1) {
		switch (opt) {
			case 'V':
				// version
				break;
			case 'w':
				timeout = parse_long(optarg, "seconds");
				break;
			default:
			case '?':
				if (optopt)
					fprintf(stderr, "error: invalid option -%c\n", optopt);
				else
					fprintf(stderr, "error: invalid option %s\n", argv[optind - 1]);
				print_usage_exit(PROG_FSVC, 1);
		}
	}

	argc -= optind, argv += optind;

	if (argc == 0) {
		fprintf(stderr, "error: command omitted\n");
		print_usage_exit(PROG_FSVC, 1);
	}
	for (struct ident* ident = command_names; ident->name != NULL; ident++) {
		if (streq(ident->name, argv[0])) {
			command = ident->command;
			break;
		}
	}
	if (command == NULL) {
		fprintf(stderr, "error: unknown command '%s'\n", argv[0]);
		print_usage_exit(PROG_FSVC, 1);
	}

	argc--, argv++;

	if (argc == 0) {
		fprintf(stderr, "error: at least one service must be specified\n");
		print_usage_exit(PROG_FSVC, 1);
	}

	chdir(SV_SERVICE_DIR);

	bool print_status;
	if ((print_status = command[0] == '!')) {
		command++;
	}

	for (int i = 0; i < argc; i++) {
		if ((dir = open(argv[i], O_DIRECTORY)) == -1) {
			fprintf(stderr, "warning: '%s' is not a valid directory\n", argv[i]);
			continue;
		}


		if (check_service(dir, runlevel) == -1) {
			fprintf(stderr, "warning: '%s' is not a valid service\n", argv[i]);
			continue;
		}

		if ((mod = get_mtime(dir)) == -1) {
			fprintf(stderr, "warning: cannot get modify-time of '%s'\n", argv[i]);
			continue;
		}

		if (command[0] != '\0') {
			if (send_command(dir, command, runlevel) == -1) {
				fprintf(stderr, "warning: unable to send command to '%s'\n", argv[i]);
				continue;
			}
		} else {
			mod++;    // avoid modtime timeout
		}

		start = time(NULL);
		if (print_status) {
			while (get_mtime(dir) == mod && time(NULL) - start < timeout)
				usleep(500);    // sleep half a secound

			if (get_mtime(dir) == mod)
				printf("timeout: ");

			printf("%s: ", progname(argv[i]));

			if (status(dir) == -1)
				printf("unable to access supervise/status\n");
		}
	}
}
