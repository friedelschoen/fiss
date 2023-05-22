#include "parse.h"

#include <fcntl.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


void parse_param_file(service_t* s, char* args[]) {
	int  param_file;
	int  args_size = 0;
	int  line_size = 0;
	char c;

	strcpy(args[args_size++], "./run");

	bool start = true;
	if ((param_file = openat(s->dir, "params", O_RDONLY)) != -1) {
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
	int  env_file;
	int  env_size  = 0;
	int  line_size = 0;
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
	int pid_file;
	if ((pid_file = openat(s->dir, "pid", O_RDONLY)) == -1)
		return 0;

	char buffer[SV_PID_BUFFER];
	int  n;
	if ((n = read(pid_file, buffer, sizeof(buffer))) <= 0) {
		close(pid_file);
		return 0;
	}
	buffer[n] = '\0';

	close(pid_file);
	return atoi(buffer);
}

/* uid:gid[:gid[:gid]...] */
static int parse_ugid_num(char* str, uid_t* uid, gid_t* gids) {
	int i;

	char* end;
	*uid = strtoul(str, &end, 10);

	if (*end != ':')
		return -1;

	str = end + 1;
	for (i = 0; i < 60; ++i, ++str) {
		gids[i++] = strtoul(str, &end, 10);

		if (*end != ':')
			break;

		str = end + 1;
	}

	if (*str != '\0')
		return -1;

	return i;
}

int parse_ugid(char* str, uid_t* uid, gid_t* gids) {
	struct passwd* pwd;
	struct group*  gr;
	char*          end;
	char*          groupstr = NULL;
	int            gid_size = 0;

	if (str[0] == ':')
		return parse_ugid_num(str + 1, uid, gids);

	if ((end = strchr(str, ':')) != NULL) {
		end[0]   = '\0';
		groupstr = end + 1;
	}

	if ((pwd = getpwnam(str)) == NULL) {
		return -1;
	}
	*uid = pwd->pw_uid;

	if (groupstr == NULL) {
		gids[0] = pwd->pw_gid;
		return 1;
	}

	char* next = groupstr;

	while (next && gid_size < 60) {
		groupstr = next;
		if ((end = strchr(groupstr, ':')) != NULL) {
			end[0] = '\0';
			next   = end + 1;
		} else {
			next = NULL;
		}
		if ((gr = getgrnam(groupstr)) == NULL)
			return -1;

		gids[gid_size++] = gr->gr_gid;
	}

	return gid_size;
}
