#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


ssize_t dgetline(int fd, char* line, size_t line_buffer) {
	ssize_t line_size = 0;
	ssize_t rc;
	char    c;
	while (line_size < (ssize_t) line_buffer - 1 && (rc = read(fd, &c, 1)) == 1) {
		if (c == '\r')
			continue;
		if (c == '\n')
			break;
		line[line_size++] = c;
	}
	line[line_size] = '\0';
	if (rc == -1 && line_size == 0)
		return -1;
	return line_size;
}

ssize_t readstr(int fd, char* str) {
	ssize_t len = 0;
	int     rc;

	while ((rc = read(fd, &str[len], 1)) == 1 && str[len] != '\0')
		len++;

	str[len] = '\0';
	return rc == -1 ? -1 : len;
}

ssize_t writestr(int fd, const char* str) {
	if (str == NULL)
		return write(fd, "", 1);
	return write(fd, str, strlen(str) + 1);
}

unsigned int stat_mode(const char* format, ...) {
	char        path[PATH_MAX];
	struct stat st;

	va_list args;
	va_start(args, format);
	vsnprintf(path, PATH_MAX, format, args);
	va_end(args);

	if (stat(path, &st) == 0)
		return st.st_mode;

	return 0;
}

int fork_dup_cd_exec(int dir, const char* path, int fd0, int fd1, int fd2) {
	pid_t pid;
	if ((pid = fork()) == -1) {
		print_error("error: cannot fork process: %s\n");
		return -1;
	} else if (pid == 0) {
		dup2(fd0, STDIN_FILENO);
		dup2(fd1, STDOUT_FILENO);
		dup2(fd2, STDERR_FILENO);

		fchdir(dir);

		execl(path, path, NULL);
		print_error("error: cannot execute stop process: %s\n");
		_exit(1);
	}
	return pid;
}

int reclaim_console(void) {
	int ttyfd;
	if ((ttyfd = open("/dev/console", O_RDWR)) == -1)
		return -1;

	dup2(ttyfd, 0);
	dup2(ttyfd, 1);
	dup2(ttyfd, 2);
	if (ttyfd > 2)
		close(ttyfd);

	return 0;
}
