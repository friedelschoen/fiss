#include "message.h"

#include "config.h"

#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>

static const char* prog_usage[] = {
	[PROG_FINIT]    = "init <0|6>",
	[PROG_FSVC]     = "fsvc <command> [-v --verbose] [-V --version] [-r --runlevel <level>] [-s --service-dir <path>]\n"
	                  "  fsvc start [-p --pin] <service>\n"
	                  "  fsvc stop [-p --pin] <service>\n"
	                  "  fsvc enable [-o --once] <service>\n"
	                  "  fsvc disable [-o --once] <service>\n"
	                  "  fsvc kill <service> <signal|signum>\n"
	                  "  fsvc status [-c --check] <service>\n"
	                  "  fsvc pause <service>\n"
	                  "  fsvc resume <service>\n"
	                  "  fsvc switch [-f --reset] <runlevel>",
	[PROG_FSVS]     = "fsvs [-V --version] [-v --verbose] [-f --force] <service-dir> <runlevel>",
	[PROG_HALT]     = "halt [-n] [-f] [-d] [-w] [-B]",
	[PROG_POWEROFF] = "poweroff [-n] [-f] [-d] [-w] [-B]",
	[PROG_REBOOT]   = "reboot [-n] [-f] [-d] [-w] [-B]",
	[PROG_SEEDRNG]  = "seedrng",
	[PROG_SIGREMAP] = "sigremap [-s --single] [-v --verbose] [-V --version] <old-signal=new-signal...> <command> [args...]",
	[PROG_VLOGGER]  = "vlogger [-isS] [-f file] [-p pri] [-t tag] [message ...]",
	[PROG_ZZZ]      = "zzz [-n --noop] [-S --freeze] [-z --suspend] [-Z --hibernate] [-R --reboot] [-H --hybrid]"
};

static const char* prog_manual[] = {
	[PROG_FINIT]    = "finit 8",
	[PROG_FSVC]     = "fsvc 8",
	[PROG_FSVS]     = "fsvs 8",
	[PROG_HALT]     = "halt 8",
	[PROG_POWEROFF] = "poweroff 8",
	[PROG_REBOOT]   = "reboot 8",
	[PROG_SEEDRNG]  = "seedrng 8",
	[PROG_SIGREMAP] = "sigremap 8",
	[PROG_VLOGGER]  = "vlogger 1",
	[PROG_ZZZ]      = "zzz 8"
};

void print_usage_exit(prog_t prog, int status) {
	fprintf(status ? stderr : stdout, "Usage: %s\n\nCheck manual '%s' for more information.\n", prog_usage[prog], prog_manual[prog]);
	exit(status);
}

void print_version_exit(void) {
	printf(SV_VERSION);
	exit(0);
}
