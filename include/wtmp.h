#pragma once

#ifndef OUR_WTMP
#	define OUR_WTMP "/var/log/wtmp"
#endif

#ifndef OUR_UTMP
#	define OUR_UTMP "/run/utmp"
#endif

void write_wtmp(int boot);