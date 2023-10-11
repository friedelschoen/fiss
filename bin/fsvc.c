// +objects: message.o util.o signame.o

#include "config.h"
#include "message.h"
#include "service.h"
#include "signame.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


const char* current_prog(void) {
	return "fsvc";
}

static const char* const command_names[][2] = {
	{ "up", "u" },           // starts the services, pin as started
	{ "down", "d" },         // stops the service, pin as stopped
	{ "once", "o" },         // same as xup
	{ "term", "t" },         // same as down
	{ "kill", "k" },         // sends kill, pin as stopped
	{ "pause", "p" },        // pauses the service
	{ "cont", "c" },         // resumes the service
	{ "reset", "r" },        // resets the service
	{ "alarm", "a" },        // sends alarm
	{ "hup", "h" },          // sends hup
	{ "int", "i" },          // sends interrupt
	{ "quit", "q" },         // sends quit
	{ "1", "1" },            // sends usr1
	{ "2", "2" },            // sends usr2
	{ "usr1", "1" },         // sends usr1
	{ "usr2", "2" },         // sends usr2
	{ "exit", "x" },         // does nothing
	{ "restart", "!du" },    // restarts the service, don't modify pin
	{ "start", "!u" },       // start the service, pin as started, print status
	{ "stop", "!d" },        // stop the service, pin as stopped, print status
	{ "status", "!" },       // print status
	{ "check", "!" },        // print status
	{ "enable", "!.e" },     // enable service
	{ "disable", "!.d" },    // disable service
	{ 0, 0 }
};

static const struct option long_options[] = {
	{ "version", no_argument, NULL, 'V' },
	{ "wait", no_argument, NULL, 'w' },
	{ 0 }
};

struct service_decode {
	int     state;
	pid_t   pid;
	time_t  state_change;
	bool    restart;
	bool    once;
	bool    is_depends;
	bool    wants_up;
	int     last_exit;
	int     return_code;
	uint8_t fail_count;
	bool    paused;
	bool    is_terminating;
};

static void decode(struct service_decode* s, const struct service_serial* buffer) {
	uint64_t tai = ((uint64_t) buffer->status_change[0] << 56) |
	               ((uint64_t) buffer->status_change[1] << 48) |
	               ((uint64_t) buffer->status_change[2] << 40) |
	               ((uint64_t) buffer->status_change[3] << 32) |
	               ((uint64_t) buffer->status_change[4] << 24) |
	               ((uint64_t) buffer->status_change[5] << 16) |
	               ((uint64_t) buffer->status_change[6] << 8) |
	               ((uint64_t) buffer->status_change[7] << 0);

	s->state_change = tai - 4611686018427387914ULL;

	s->state          = buffer->state;
	s->return_code    = buffer->return_code;
	s->fail_count     = buffer->fail_count;
	s->is_terminating = (buffer->flags >> 4) & 0x01;
	s->once           = (buffer->flags >> 3) & 0x01;
	s->restart        = (buffer->flags >> 2) & 0x01;
	s->last_exit      = (buffer->flags >> 0) & 0x03;

	s->pid = (buffer->pid[0] << 0) |
	         (buffer->pid[1] << 8) |
	         (buffer->pid[2] << 16) |
	         (buffer->pid[3] << 24);

	s->paused   = buffer->paused;
	s->wants_up = buffer->restart == 'u';

	s->is_depends = s->wants_up != (s->once || s->restart);
}

static time_t get_mtime(int dir) {
	struct stat st;
	if (fstatat(dir, "supervise/status", &st, 0) == -1)
		return -1;
	return st.st_mtim.tv_sec;
}

static int handle_command(int dir, char command) {
	// no custom commands defined

	(void) dir, (void) command;

	return -1;
}

static int send_command(int dir, const char* command) {
	int fd;
	if ((fd = openat(dir, "supervise/control", O_WRONLY | O_NONBLOCK)) == -1)
		return -1;

	for (const char* c = command; *c != '\0'; c++) {
		if (*c == '.') {
			c++;
			if (handle_command(dir, *c) == -1)
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
	struct service_decode s;

	if ((fd = openat(dir, "supervise/status", O_RDONLY | O_NONBLOCK)) == -1)
		return -1;

	if (read(fd, &buffer, sizeof(buffer)) == -1) {
		close(fd);
		return -1;
	}

	close(fd);

	decode(&s, &buffer);

	timeval = time(NULL) - s.state_change;

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
		case STATE_ERROR:
			printf("dead (error)");
			break;
	}

	if (s.paused)
		printf(" & paused");

	printf(" since %lu%s", timeval, timeunit);

	if (s.once == S_ONCE)
		printf(", started once");

	if (s.restart)
		printf(", should restart");

	if (s.is_depends)
		printf(", started as dependency");

	if (s.return_code > 0 && s.last_exit == EXIT_NORMAL)
		printf(", exited with %d", s.return_code);

	if (s.return_code > 0 && s.last_exit == EXIT_SIGNALED)
		printf(", crashed with SIG%s", sigabbr(s.return_code));

	if (s.fail_count > 0)
		printf(", failed %d times", s.fail_count);

	printf("\n");

	return 0;
}

int main(int argc, char** argv) {
	int opt, dir, fd,
	    timeout = SV_STATUS_WAIT;
	time_t mod, start;

	const char* command = NULL;
	const char* service;

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
	for (const char** ident = (void*) command_names; ident[0] != NULL; ident++) {
		if (streq(ident[0], argv[0])) {
			command = ident[1];
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
		service = progname(argv[i]);

		if ((dir = open(argv[i], O_DIRECTORY)) == -1) {
			fprintf(stderr, "error: %s: cannot open directory: %s\n", argv[i], strerror(errno));
			continue;
		}

		if ((fd = openat(dir, "supervise/ok", O_WRONLY | O_NONBLOCK)) == -1) {
			fprintf(stderr, "error: %s: cannot open supervise/control: %s\n", argv[i], strerror(errno));
			continue;
		}
		close(fd);

		if ((mod = get_mtime(dir)) == -1) {
			fprintf(stderr, "error: %s: cannot get modify-time\n", argv[i]);
			continue;
		}

		if (command[0] != '\0') {
			if (send_command(dir, command) == -1) {
				fprintf(stderr, "error: %s: unable to send command\n", argv[i]);
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

			printf("%s: ", service);

			if (status(dir) == -1)
				printf("unable to access supervise/status\n");
		}
	}
}
