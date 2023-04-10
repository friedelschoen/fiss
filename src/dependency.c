#include "service.h"
#include "util.h"

#include <fcntl.h>
#include <linux/limits.h>
#include <stdio.h>
#include <string.h>


void service_add_dependency(service_t* s, service_t* d) {
	if (s == d)
		return;

	depends[depends_size].service = s;
	depends[depends_size].depends = d;
	depends_size++;
}

void service_update_dependency(service_t* s) {
	service_t* dep;

	if (s->log_service) {	 // aka keep first entry (the log service) if a log service is used
		service_add_dependency(s, s->log_service);
	}

	int	 depends_file;
	char depends_path[PATH_MAX];
	snprintf(depends_path, PATH_MAX, "%s/%s/%s", service_dir, s->name, "depends");

	if ((depends_file = open(depends_path, O_RDONLY)) == -1)
		return;

	char line[512];
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
