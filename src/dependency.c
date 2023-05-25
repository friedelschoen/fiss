#include "config.h"
#include "service.h"
#include "util.h"

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


void service_add_dependency(service_t* s, service_t* d) {
	if (s == d)
		return;

	depends[depends_size][0] = s;
	depends[depends_size][1] = d;
	depends_size++;
}

void service_update_dependency(service_t* s) {
	service_t* dep;
	int        depends_file;
	char       line[SV_NAME_MAX];

	if (s->log_service) {    // aka keep first entry (the log service) if a log service is used
		service_add_dependency(s, s->log_service);
	}

	if ((depends_file = openat(s->dir, "depends", O_RDONLY)) == -1)
		return;

	while (dgetline(depends_file, line, sizeof(line)) > 0) {
		if (streq(s->name, line)) {
			fprintf(stderr, "warning: %s depends on itself\n", s->name);
			continue;
		}

		if ((dep = service_get(line)) == NULL) {
			fprintf(stderr, "warning: %s depends on %s: dependency not found\n", s->name, line);
			continue;
		}
		service_add_dependency(s, dep);
	}

	close(depends_file);
}
