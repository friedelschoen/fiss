#include "util.h"

#include <string.h>
#include <sys/socket.h>
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

ssize_t writestr(int fd, string str) {
	if (str == NULL)
		return write(fd, "", 1);
	return write(fd, str, strlen(str) + 1);
}