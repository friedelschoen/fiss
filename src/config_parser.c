#include "config_parser.h"

#include <fcntl.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>


void parse_param_file(service_t* s, char* args[]) {
	int	 param_file;
	int	 args_size = 0;
	int	 line_size = 0;
	char c;

	snprintf(args[args_size++], SV_PARAM_FILE_LINE_MAX, "%s/%s/%s", service_dir, s->name, "run");

	bool start = true;
	if ((param_file = open("params", O_RDONLY)) != -1) {
		while (read(param_file, &c, 1) > 0) {
			if (start && c == '%') {
				args_size--;
				continue;
			}
			if (c == '\n') {
				args[args_size++][line_size] = '\0';

				line_size = 0;
			} else {
				args[args_size][line_size++] = c;
			}
			start = false;
		}
		if (line_size > 0)
			args[args_size++][line_size] = '\0';
		close(param_file);
	}

	args[args_size] = NULL;
}

void parse_env_file(char** env) {
	int	 env_file;
	int	 env_size  = 0;
	int	 line_size = 0;
	char c;

	if ((env_file = open("env", O_RDONLY)) != -1) {
		while (read(env_file, &c, 1) > 0) {
			if (c == '\n') {
				env[env_size++][line_size] = '\0';

				line_size = 0;
			} else {
				env[env_size][line_size++] = c;
			}
		}
		if (line_size > 0)
			env[env_size++][line_size] = '\0';
		close(env_file);
	}

	env[env_size] = NULL;
}


pid_t parse_pid_file(service_t* s) {
	char path_buf[PATH_MAX];
	snprintf(path_buf, PATH_MAX, "%s/%s/pid", service_dir, s->name);
	int pid_file;
	if ((pid_file = open(path_buf, O_RDONLY)) == -1)
		return 0;

	char buffer[20];
	int	 n;
	if ((n = read(pid_file, buffer, sizeof(buffer))) <= 0) {
		close(pid_file);
		return 0;
	}
	buffer[n] = '\0';

	close(pid_file);
	return atoi(buffer);
}
