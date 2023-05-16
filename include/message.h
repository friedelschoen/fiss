#pragma once

#define FISS_VERSION "0.1.1"

#define FISS_VERSION_STRING "fiss version v" FISS_VERSION ""

typedef enum prog {
	PROG_FINIT,
	PROG_FSVC,
	PROG_FSVS,
	PROG_HALT,
	PROG_POWEROFF,
	PROG_REBOOT,
	PROG_SEEDRNG,
	PROG_SIGREMAP,
	PROG_VLOGGER,
	PROG_ZZZ
} prog_t;

void print_usage_exit(prog_t prog, int status) __attribute__((noreturn));
void print_version_exit(void) __attribute__((noreturn));
