#include <signal.h>
#include <unistd.h>

static void signal_nop(int signum) {
	(void) signum;
}

int main() {
	signal(SIGTERM, signal_nop);
	signal(SIGINT, signal_nop);
	signal(SIGHUP, SIG_IGN);

	pause();

	return 0;
}
