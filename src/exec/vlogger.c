#include "config.h"
#include "message.h"
#include "util.h"

#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslog.h>
#include <unistd.h>

static char pwd[PATH_MAX];

typedef struct ident {
	const char* name;
	int         value;
} ident_t;

ident_t prioritynames[] = {
	{ "alert", LOG_ALERT },
	{ "crit", LOG_CRIT },
	{ "debug", LOG_DEBUG },
	{ "emerg", LOG_EMERG },
	{ "err", LOG_ERR },
	{ "error", LOG_ERR },
	{ "info", LOG_INFO },
	{ "notice", LOG_NOTICE },
	{ "panic", LOG_EMERG },
	{ "warn", LOG_WARNING },
	{ "warning", LOG_WARNING },
	{ 0, -1 }
};

ident_t facilitynames[] = {
	{ "auth", LOG_AUTH },
	{ "authpriv", LOG_AUTHPRIV },
	{ "cron", LOG_CRON },
	{ "daemon", LOG_DAEMON },
	{ "ftp", LOG_FTP },
	{ "kern", LOG_KERN },
	{ "lpr", LOG_LPR },
	{ "mail", LOG_MAIL },
	{ "news", LOG_NEWS },
	{ "security", LOG_AUTH },
	{ "syslog", LOG_SYSLOG },
	{ "user", LOG_USER },
	{ "uucp", LOG_UUCP },
	{ "local0", LOG_LOCAL0 },
	{ "local1", LOG_LOCAL1 },
	{ "local2", LOG_LOCAL2 },
	{ "local3", LOG_LOCAL3 },
	{ "local4", LOG_LOCAL4 },
	{ "local5", LOG_LOCAL5 },
	{ "local6", LOG_LOCAL6 },
	{ "local7", LOG_LOCAL7 },
	{ 0, -1 }
};

static void strpriority(char* facil_str, int* facility, int* level) {
	char*    prio_str = NULL;
	ident_t* ident;

	if ((prio_str = strchr(facil_str, '.'))) {
		*prio_str = '\0';
		prio_str++;
		for (ident = prioritynames; ident->name; ident++) {
			if (streq(ident->name, prio_str))
				*level = ident->value;
		}
	}
	if (*facil_str) {
		for (ident = facilitynames; ident->name; ident++) {
			if (streq(ident->name, facil_str))
				*facility = ident->value;
		}
	}
}

int main(int argc, char* argv[]) {
	char  buf[SV_VLOGGER_BUFFER];
	char *p, *e, *argv0;
	char* tag = NULL;
	int   c;
	bool  Sflag    = false;
	int   logflags = 0;
	int   facility = LOG_USER;
	int   level    = LOG_NOTICE;

	argv0 = *argv;

	if (streq(argv0, "./run")) {
		// if running as a service, update facility and tag
		p = getcwd(pwd, sizeof(pwd));
		if (p != NULL && *pwd == '/') {
			if (*(p = pwd + (strlen(pwd) - 1)) == '/')
				*p = '\0';
			if ((p = strrchr(pwd, '/')) && strncmp(p + 1, "log", 3) == 0 &&
			    (*p = '\0', (p = strrchr(pwd, '/'))) && (*(p + 1) != '\0')) {
				tag      = p + 1;
				facility = LOG_DAEMON;
				level    = LOG_NOTICE;
			}
		}
	} else if (streq(basename(argv0), "logger")) {
		/* behave just like logger(1) and only use syslog */
		Sflag = true;
	}

	while ((c = getopt(argc, argv, "f:ip:Sst:")) != -1)
		switch (c) {
			case 'f':
				if (freopen(optarg, "r", stdin) == NULL) {
					print_error("error: unable to reopen %s: %s\n", optarg);
					return 1;
				}
				break;
			case 'i':
				logflags |= LOG_PID;
				break;
			case 'p':
				strpriority(optarg, &facility, &level);
				break;
			case 'S':
				Sflag = true;
				break;
			case 's':
				logflags |= LOG_PERROR;
				break;
			case 't':
				tag = optarg;
				break;
			default:
				print_usage_exit(PROG_VLOGGER, 1);
		}
	argc -= optind;
	argv += optind;

	if (argc > 0)
		Sflag = true;

	if (!Sflag && access("/etc/vlogger", X_OK) != -1) {
		ident_t*    ident;
		const char *sfacility = "", *slevel = "";
		for (ident = prioritynames; ident->name; ident++) {
			if (ident->value == level)
				slevel = ident->name;
		}
		for (ident = facilitynames; ident->name; ident++) {
			if (ident->value == facility)
				sfacility = ident->name;
		}
		execl("/etc/vlogger", argv0, tag ?: "", slevel, sfacility, NULL);
		print_error("error: unable to exec /etc/vlogger: %s\n");
		exit(1);
	}

	openlog(tag ?: getlogin(), logflags, facility);

	if (argc > 0) {
		size_t len;
		p  = buf;
		*p = '\0';
		e  = buf + sizeof(buf) - 2;
		while (*argv) {
			len = strlen(*argv);
			if (p + len > e && p > buf) {
				syslog(level | facility, "%s", buf);
				p  = buf;
				*p = '\0';
			}
			if (len > sizeof(buf) - 1) {
				syslog(level | facility, "%s", *argv++);
			} else {
				if (p != buf) {
					*p++ = ' ';
					*p   = '\0';
				}
				strncat(p, *argv++, e - p);
				p += len;
			}
		}
		if (p != buf)
			syslog(level | facility, "%s", buf);
		return 0;
	}

	while (fgets(buf, sizeof(buf), stdin) != NULL)
		syslog(level | facility, "%s", buf);

	return 0;
}
