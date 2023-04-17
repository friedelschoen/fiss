#pragma once

#include <time.h>
#include <unistd.h>

#define streq(a, b)         (!strcmp((a), (b)))
#define stringify(s)        #s
#define static_stringify(s) stringify(s)

#define print_error(msg, ...)   (fprintf(stderr, "error: " msg ": %s\n", ##__VA_ARGS__, strerror(errno)))
#define print_warning(msg, ...) (fprintf(stderr, "warning: " msg ": %s\n", ##__VA_ARGS__, strerror(errno)))

typedef struct {
	int read;
	int write;
} pipe_t;

ssize_t dgetline(int fd, char* line, size_t line_buffer);
ssize_t readstr(int fd, char* str);
ssize_t writestr(int fd, const char* str);