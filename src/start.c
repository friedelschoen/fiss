#include "config.h"
#include "config_parser.h"
#include "service.h"
#include "user_group.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


static void set_pipes(service_t* s) {
	struct stat estat;

	if (s->is_log_service) {
		close(s->log_pipe.write);
		dup2(s->log_pipe.read, STDIN_FILENO);
		close(s->log_pipe.read);
		dup2(null_fd, STDOUT_FILENO);
		dup2(null_fd, STDERR_FILENO);
	} else if (s->log_service) {	// aka has_log_service
		close(s->log_service->log_pipe.read);
		dup2(s->log_service->log_pipe.write, STDOUT_FILENO);
		dup2(s->log_service->log_pipe.write, STDERR_FILENO);
		close(s->log_service->log_pipe.write);
		dup2(null_fd, STDIN_FILENO);
	} else if (stat("log", &estat) == 0 && estat.st_mode & S_IWRITE) {	  // is not
		int log_fd;
		if ((log_fd = open("log", O_WRONLY | O_TRUNC)) == -1)
			log_fd = null_fd;

		dup2(null_fd, STDIN_FILENO);
		dup2(log_fd, STDOUT_FILENO);
		dup2(log_fd, STDERR_FILENO);
	} else if (stat("nolog", &estat) == 0 && S_ISREG(estat.st_mode)) {
		dup2(null_fd, STDIN_FILENO);
		dup2(null_fd, STDOUT_FILENO);
		dup2(null_fd, STDERR_FILENO);
	} else {
		char service_log[PATH_MAX];
		snprintf(service_log, PATH_MAX, "%s/%s-%s.log", SV_LOG_DIR, s->name, runlevel);

		int log_fd;
		if ((log_fd = open(service_log, O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1)
			log_fd = null_fd;

		dup2(null_fd, STDIN_FILENO);
		dup2(log_fd, STDOUT_FILENO);
		dup2(log_fd, STDERR_FILENO);
	}
}

static void set_user() {
	char buffer[1024];
	int	 user_file;
	if ((user_file = open("user", O_RDONLY)) != -1) {
		ssize_t n;
		if ((n = read(user_file, buffer, sizeof(buffer))) == -1) {
			printf("failed reading user\n");
			close(user_file);
			return;
		}
		buffer[n] = '\0';

		uid_t uid;
		gid_t gids[60];
		if ((n = parse_ugid(buffer, &uid, gids)) <= 0) {
			printf("error parsing user\n");
			close(user_file);
			return;
		}

		setgroups(n, gids);
		setgid(gids[0]);
		setuid(uid);

		close(user_file);
	}
}


void service_start(service_t* s, bool* changed) {
	if (s->state != STATE_INACTIVE)
		return;

	if (changed)
		*changed = true;

	printf(":: starting %s \n", s->name);
	for (int i = 0; i < depends_size; i++) {
		if (depends[i].service == s)
			service_start(depends[i].depends, NULL);
	}

	for (int i = 0; i < depends_size; i++) {
		if (depends[i].service == s)
			service_start(depends[i].depends, NULL);
	}

	char path_buf[PATH_MAX];

	struct stat estat;
	if (sprintf(path_buf, "%s/%s/run", service_dir, s->name) && stat(path_buf, &estat) == 0 && estat.st_mode & S_IXUSR) {
		s->state = STATE_ACTIVE_FOREGROUND;
	} else if (sprintf(path_buf, "%s/%s/start", service_dir, s->name) && stat(path_buf, &estat) == 0 && estat.st_mode & S_IXUSR) {
		s->state = STATE_STARTING;
	} else if (sprintf(path_buf, "%s/%s/depends", service_dir, s->name) && stat(path_buf, &estat) == 0 && estat.st_mode & S_IREAD) {
		s->state = STATE_ACTIVE_DUMMY;
	} else {
		printf("error in %s: `run`, `start` or `depends` not found\n", s->name);
	}

	if (s->state != STATE_ACTIVE_DUMMY) {
		if ((s->pid = fork()) == -1) {
			print_error("cannot fork process");
			exit(1);
		} else if (s->pid == 0) {	 // child
			if (setsid() == -1)
				print_error("cannot setsid");

			char dir_path[PATH_MAX];
			snprintf(dir_path, PATH_MAX, "%s/%s", service_dir, s->name);
			if (chdir(dir_path) == -1)
				print_error("chdir failed");

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
			print_error("cannot execute service");
			_exit(1);
		}
	}
	s->status_change = time(NULL);
	printf(":: started %s \n", s->name);
}
