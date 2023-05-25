#include "parse.h"
#include "service.h"

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


static void set_pipes(service_t* s) {
	if (s->is_log_service) {
		close(s->log_pipe.write);
		dup2(s->log_pipe.read, STDIN_FILENO);
		close(s->log_pipe.read);
		dup2(null_fd, STDOUT_FILENO);
		dup2(null_fd, STDERR_FILENO);
	} else if (s->log_service) {    // aka has_log_service
		close(s->log_service->log_pipe.read);
		dup2(s->log_service->log_pipe.write, STDOUT_FILENO);
		dup2(s->log_service->log_pipe.write, STDERR_FILENO);
		close(s->log_service->log_pipe.write);
		dup2(null_fd, STDIN_FILENO);
	} else if (stat_mode("log") & S_IWRITE) {    // is not
		int log_fd;
		if ((log_fd = open("log", O_WRONLY | O_TRUNC)) == -1)
			log_fd = null_fd;

		dup2(null_fd, STDIN_FILENO);
		dup2(log_fd, STDOUT_FILENO);
		dup2(log_fd, STDERR_FILENO);
	} else if (S_ISREG(stat_mode("nolog"))) {
		dup2(null_fd, STDIN_FILENO);
		dup2(null_fd, STDOUT_FILENO);
		dup2(null_fd, STDERR_FILENO);
	} else {
		char service_log[PATH_MAX];
		int  log_fd;

		snprintf(service_log, PATH_MAX, "%s/%s-%s.log", SV_LOG_DIR, s->name, runlevel);

		if ((log_fd = open(service_log, O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1)
			log_fd = null_fd;

		dup2(null_fd, STDIN_FILENO);
		dup2(log_fd, STDOUT_FILENO);
		dup2(log_fd, STDERR_FILENO);
	}
}

static void set_user(void) {
	char    buffer[SV_USER_BUFFER];
	int     user_file;
	ssize_t n;
	uid_t   uid;
	gid_t   gids[SV_USER_GROUP_MAX];

	if ((user_file = open("user", O_RDONLY)) == -1)
		return;

	if ((n = read(user_file, buffer, sizeof(buffer))) == -1) {
		print_error("error: failed reading ./user: %s\n");
		close(user_file);
		return;
	}
	buffer[n] = '\0';

	if ((n = parse_ugid(buffer, &uid, gids)) <= 0) {
		fprintf(stderr, "warn: malformatted user file\n");
		close(user_file);
		return;
	}

	setgroups(n, gids);
	setgid(gids[0]);
	setuid(uid);

	close(user_file);
}

void service_run(service_t* s) {
	struct stat st;

	if (fstatat(s->dir, "run", &st, 0) != -1 && st.st_mode & S_IXUSR) {
		s->state = STATE_ACTIVE_FOREGROUND;
	} else if (fstatat(s->dir, "start", &st, 0) != -1 && st.st_mode & S_IXUSR) {
		s->state = STATE_STARTING;
	} else if (fstatat(s->dir, "depends", &st, 0) != -1 && st.st_mode & S_IREAD) {
		s->state = STATE_ACTIVE_DUMMY;
	} else {
		fprintf(stderr, "warn: %s: `run`, `start` or `depends` not found\n", s->name);
		s->state = STATE_INACTIVE;
	}

	if (s->state != STATE_ACTIVE_DUMMY) {
		if ((s->pid = fork()) == -1) {
			print_error("error: cannot fork process: %s\n");
			exit(1);
		} else if (s->pid == 0) {    // child
			if (setsid() == -1)
				print_error("error: cannot setsid: %s\n");

			fchdir(s->dir);
			set_pipes(s);

			char  args[SV_ARGUMENTS_MAX][SV_PARAM_FILE_LINE_MAX];
			char* argv[SV_ARGUMENTS_MAX];
			for (int i = 0; i < SV_ARGUMENTS_MAX; i++)
				argv[i] = args[i];

			char  envs[SV_ENV_MAX][SV_ENV_FILE_LINE_MAX];
			char* envv[SV_ENV_MAX];
			for (int i = 0; i < SV_ENV_MAX; i++)
				envv[i] = envs[i];

			parse_param_file(s, argv);
			parse_env_file(envv);

			set_user();

			if (s->state == STATE_STARTING) {
				execve("./start", argv, envv);
			} else {
				execve("./run", argv, envv);
			}
			print_error("error: cannot execute service: %s\n");
			_exit(1);
		}
	}
	s->status_change = time(NULL);
	service_update_status(s);
}

void service_start(service_t* s) {
	struct stat st;

	if (s->state != STATE_INACTIVE)
		return;

	printf("starting %s\n", s->name);
	for (int i = 0; i < depends_size; i++) {
		if (depends[i][0] == s)
			service_start(depends[i][1]);
	}

	if (fstatat(s->dir, "setup", &st, 0) != -1 && st.st_mode & S_IXUSR) {
		s->state = STATE_SETUP;
		if ((s->pid = fork_dup_cd_exec(s->dir, "./setup", null_fd, null_fd, null_fd)) == -1) {
			print_error("error: cannot execute ./setup: %s\n");
			s->state = STATE_INACTIVE;
		}
		s->status_change = time(NULL);
		service_update_status(s);
	} else {
		service_run(s);
	}
	printf("started %s \n", s->name);
}
