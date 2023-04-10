#include "pattern.h"

static const char* strend(const char* s) {
	while (*s)
		s++;
	return s;
}

int pattern_test(const char* pattern, const char* match) {
	int         i = 0;
	const char *p, *m, *b;

	// if more than one '*': exit with error
	for (p = pattern; *p != '\0'; p++) {
		if (*p == '*' && i++)
			return -1;
	}

	m = match;
	for (p = pattern; *p != '\0' && *p != '*'; p++, m++) {
		if (*m == '\0' || (*p != *m && *p != '%'))
			return 0;
	}

	if (*p == '\0' && *m != '\0')
		return 0;

	if (*p == '*') {
		b = m;
		m = strend(match);
		for (p = strend(pattern); p >= pattern && *p != '*'; p--, m--) {
			if (m < b || (*p != *m && *p != '%'))
				return 0;
		}
	}

	return 1;
}