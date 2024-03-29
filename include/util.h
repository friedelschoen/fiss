#pragma once

#include <stdio.h>

#define streq(a, b) (!strcmp((a), (b)))

#define print_errno(msg, ...) (fprintf(stderr, "%s: " msg, current_prog(), ##__VA_ARGS__, strerror(errno)))
#define print_error(msg, ...) (fprintf(stderr, "%s: " msg, current_prog(), ##__VA_ARGS__))

typedef struct {
	int read;
	int write;
} pipe_t;

const char* current_prog(void);    // has to be defined per program

ssize_t      dgetline(int fd, char* line, size_t line_buffer);
ssize_t      readstr(int fd, char* str);
ssize_t      writestr(int fd, const char* str);
unsigned int stat_mode(const char* format, ...);
int          fork_dup_cd_exec(int dir, const char* path, int fd0, int fd1, int fd2);
int          reclaim_console(void);
void         sigblock_all(int unblock);
long         parse_long(const char* str, const char* name);
char*        progname(char* path);
int          fd_set_flag(int fd, int flags);
