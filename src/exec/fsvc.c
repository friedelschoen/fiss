#include "config.h"
#include "message.h"
#include "service.h"
#include "util.h"

#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


struct ident {
	const char* name;
	const char* command;
};

static struct ident command_names[] = {
	{ "up", "u" },             // starts the services, pin as started
	{ "down", "d" },           // stops the service, pin as stopped
	{ "xup", "U" },            // stops the service, don't pin as stopped
	{ "xdown", "D" },          // stops the service, don't pin as stopped
	{ "once", "o" },           // same as xup
	{ "term", "t" },           // same as down
	{ "kill", "k" },           // sends kill, pin as stopped
	{ "pause", "p" },          // pauses the service
	{ "cont", "c" },           // resumes the service
	{ "reset", "r" },          // resets the service
	{ "alarm", "a" },          // sends alarm
	{ "hup", "h" },            // sends hup
	{ "int", "i" },            // sends interrupt
	{ "quit", "q" },           // sends quit
	{ "1", "1" },              // sends usr1
	{ "2", "2" },              // sends usr2
	{ "exit", "x" },           // does nothing
	{ "+up", "U0" },           // starts the service, don't modify pin
	{ "+down", "D0" },         // stops the service, don't modify pin
	{ "restart", "!D0U0" },    // restarts the service, don't modify pin
	{ "start", "!u" },         // start the service, pin as started, print status
	{ "stop", "!d" },          // stop the service, pin as stopped, print status
	{ "status", "!" },         // print status
	{ 0, 0 }
};

static const struct option long_options[] = {
	{ "version", no_argument, NULL, 'V' },
	{ "wait", no_argument, NULL, 'w' },
	{ 0 }
};

static char* progname(char* path) {
	char* match;
	for (;;) {
		if ((match = strrchr(path, '/')) == NULL)
			return path;

		if (match[1] != '\0')
			return match + 1;

		*match = '\0';
	}
	return path;
}

static int check_service(int dir) {
	int fd;
	if ((fd = openat(dir, "supervise/ok", O_WRONLY | O_NONBLOCK)) == -1)
		return -1;
	close(fd);
	return 0;
}

static time_t get_mtime(int dir) {
	struct stat st;
	if (fstatat(dir, "supervise/status", &st, 0) == -1)
		return -1;
	return st.st_mtim.tv_sec;
}

static int send_command(int dir, const char* command) {
	int fd;
	if ((fd = openat(dir, "supervise/control", O_WRONLY | O_NONBLOCK)) == -1)
		return -1;

	if (write(fd, command, strlen(command)) == -1)
		return -1;

	close(fd);

	return 0;
}

int status(int dir) {
	int fd;
	if ((fd = openat(dir, "supervise/status", O_RDONLY | O_NONBLOCK)) == -1)
		return -1;

	service_serial_t buffer;
	service_t        s;

	if (read(fd, &buffer, sizeof(buffer)) == -1) {
		close(fd);
		return -1;
	}

	close(fd);

	service_decode(&s, &buffer);

	printf("%d\n", s.pid);

	return 0;
}

int main(int argc, char** argv) {
	int opt;
	int timeout = SV_STATUS_WAIT;

	const char* command = NULL;

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

	int    dir;
	time_t mod, start;
	for (int i = 0; i < argc; i++) {
		if ((dir = open(argv[i], O_DIRECTORY)) == -1) {
			fprintf(stderr, "warning: '%s' is not a valid directory\n", argv[0]);
			continue;
		}

		if (check_service(dir) == -1) {
			fprintf(stderr, "warning: '%s' is not a valid service\n", argv[0]);
			continue;
		}

		if ((mod = get_mtime(dir)) == -1) {
			fprintf(stderr, "warning: cannot get modify-time of '%s'\n", argv[0]);
			continue;
		}

		if (command[0] != '\0') {
			if (send_command(dir, command) == -1) {
				fprintf(stderr, "warning: unable to send command to '%s'\n", argv[0]);
				continue;
			}
		} else {
			mod++;    // avoid modtime timeout
		}

		start = time(NULL);
		if (print_status) {
			while (get_mtime(dir) == mod && time(NULL) - start < timeout)
				usleep(500);    // sleep half a secound

			printf(get_mtime(dir) == mod ? "timeout: %s: " : "ok: %s: ", progname(argv[i]));
			if (status(dir) == -1)
				printf("unable to access supervise/status\n");
		}
	}
}
