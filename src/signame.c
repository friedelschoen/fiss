#include "signame.h"

#include "util.h"

#include <signal.h>
#include <stdlib.h>
#include <string.h>


#define SIGNUM_NAME(name) \
	{ SIG##name, #name }

typedef struct signal_name {
	int         num;
	const char* name;
} signal_name_t;

static signal_name_t signal_names[] = {
/* Signals required by POSIX 1003.1-2001 base, listed in
   traditional numeric order where possible.  */
#ifdef SIGHUP
	SIGNUM_NAME(HUP),
#endif
#ifdef SIGINT
	SIGNUM_NAME(INT),
#endif
#ifdef SIGQUIT
	SIGNUM_NAME(QUIT),
#endif
#ifdef SIGILL
	SIGNUM_NAME(ILL),
#endif
#ifdef SIGTRAP
	SIGNUM_NAME(TRAP),
#endif
#ifdef SIGABRT
	SIGNUM_NAME(ABRT),
#endif
#ifdef SIGFPE
	SIGNUM_NAME(FPE),
#endif
#ifdef SIGKILL
	SIGNUM_NAME(KILL),
#endif
#ifdef SIGSEGV
	SIGNUM_NAME(SEGV),
#endif
/* On Haiku, SIGSEGV == SIGBUS, but we prefer SIGSEGV to match
   strsignal.c output, so SIGBUS must be listed second.  */
#ifdef SIGBUS
	SIGNUM_NAME(BUS),
#endif
#ifdef SIGPIPE
	SIGNUM_NAME(PIPE),
#endif
#ifdef SIGALRM
	SIGNUM_NAME(ALRM),
#endif
#ifdef SIGTERM
	SIGNUM_NAME(TERM),
#endif
#ifdef SIGUSR1
	SIGNUM_NAME(USR1),
#endif
#ifdef SIGUSR2
	SIGNUM_NAME(USR2),
#endif
#ifdef SIGCHLD
	SIGNUM_NAME(CHLD),
#endif
#ifdef SIGURG
	SIGNUM_NAME(URG),
#endif
#ifdef SIGSTOP
	SIGNUM_NAME(STOP),
#endif
#ifdef SIGTSTP
	SIGNUM_NAME(TSTP),
#endif
#ifdef SIGCONT
	SIGNUM_NAME(CONT),
#endif
#ifdef SIGTTIN
	SIGNUM_NAME(TTIN),
#endif
#ifdef SIGTTOU
	SIGNUM_NAME(TTOU),
#endif

/* Signals required by POSIX 1003.1-2001 with the XSI extension.  */
#ifdef SIGSYS
	SIGNUM_NAME(SYS),
#endif
#ifdef SIGPOLL
	SIGNUM_NAME(POLL),
#endif
#ifdef SIGVTALRM
	SIGNUM_NAME(VTALRM),
#endif
#ifdef SIGPROF
	SIGNUM_NAME(PROF),
#endif
#ifdef SIGXCPU
	SIGNUM_NAME(XCPU),
#endif
#ifdef SIGXFSZ
	SIGNUM_NAME(XFSZ),
#endif

/* Unix Version 7.  */
#ifdef SIGIOT
	SIGNUM_NAME(IOT), /* Older name for ABRT.  */
#endif
#ifdef SIGEMT
	SIGNUM_NAME(EMT),
#endif

/* USG Unix.  */
#ifdef SIGPHONE
	SIGNUM_NAME(PHONE),
#endif
#ifdef SIGWIND
	SIGNUM_NAME(WIND),
#endif

/* Unix System V.  */
#ifdef SIGCLD
	SIGNUM_NAME(CLD),
#endif
#ifdef SIGPWR
	SIGNUM_NAME(PWR),
#endif

/* GNU/Linux 2.2 and Solaris 8.  */
#ifdef SIGCANCEL
	SIGNUM_NAME(CANCEL),
#endif
#ifdef SIGLWP
	SIGNUM_NAME(LWP),
#endif
#ifdef SIGWAITING
	SIGNUM_NAME(WAITING),
#endif
#ifdef SIGFREEZE
	SIGNUM_NAME(FREEZE),
#endif
#ifdef SIGTHAW
	SIGNUM_NAME(THAW),
#endif
#ifdef SIGLOST
	SIGNUM_NAME(LOST),
#endif
#ifdef SIGWINCH
	SIGNUM_NAME(WINCH),
#endif

/* GNU/Linux 2.2.  */
#ifdef SIGINFO
	SIGNUM_NAME(INFO),
#endif
#ifdef SIGIO
	SIGNUM_NAME(IO),
#endif
#ifdef SIGSTKFLT
	SIGNUM_NAME(STKFLT),
#endif

/* OpenBSD.  */
#ifdef SIGTHR
	SIGNUM_NAME(THR),
#endif

	{ 0, NULL },
};

int signame(char const* name) {
	char* endptr;
	int   signum;

	if ((signum = strtol(name, &endptr, 10)) && endptr == strchr(name, '\0'))
		return signum;

	// startswith SIG, remove that so -SIGKILL == -KILL
	if (strncmp(name, "SIG", 3) == 0) {
		name += 3;
	}

	// search for name
	for (signal_name_t* sigpair = signal_names; sigpair->num != 0; sigpair++)
		if (streq(sigpair->name, name))
			return sigpair->num;

	return -1;
}

const char* sigabbr(int signal) {
	// search for name
	for (signal_name_t* sigpair = signal_names; sigpair->num != 0; sigpair++)
		if (sigpair->num == signal)
			return sigpair->name;

	return "UNKNOWN";
}
