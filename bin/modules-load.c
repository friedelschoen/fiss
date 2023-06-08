// +objects: util.o

#include "util.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_CMDLINE_SIZE 4096
#define MAX_MODULE_SIZE  256
#define MAX_MODULE_COUNT 64


static char kernel_cmdline[MAX_CMDLINE_SIZE];
static char modules[MAX_MODULE_COUNT][MAX_MODULE_SIZE];
static int  modules_size = 0;

static void read_cmdline(void) {
	int   fd;
	int   size;
	char *end, *match, *com;

	if ((fd = open("/proc/cmdline", O_RDONLY)) == -1)
		return;

	if ((size = read(fd, kernel_cmdline, sizeof(kernel_cmdline))) == -1) {
		print_error("cannot read /proc/cmdline: %s\n");
		close(fd);
		return;
	}
	kernel_cmdline[size] = '\0';

	end = kernel_cmdline;

	while (end < kernel_cmdline + size && (match = strstr(end, "modules-load=")) != NULL) {
		if (match != end && match[-1] != '.' && match[-1] != ' ') {
			end += sizeof("modules-load=") - 1;    // -1 because of implicit '\0'
			continue;
		}

		match += sizeof("modules-load=") - 1;    // -1 because of implicit '\0'
		if ((end = strchr(match, ' ')) == NULL)
			end = kernel_cmdline + size;
		*end = '\0';

		while ((com = strchr(match, ',')) != NULL) {
			*com = '\0';
			strcpy(modules[modules_size++], match);
			match = com + 1;
		}
		if (match[0] != '\0')
			strcpy(modules[modules_size++], match);
	}
}

static void read_file(const char* path) {
	int   fd;
	char  line[MAX_MODULE_SIZE];
	char* comment;

	if ((fd = open(path, O_RDONLY)) == -1) {
		print_error("unable to open %s: %s\n", path);
		return;
	}

	while (dgetline(fd, line, sizeof(line)) > 0) {
		if ((comment = strchr(line, '#')) != NULL) {
			*comment = '\0';
		}
		if ((comment = strchr(line, ';')) != NULL) {
			*comment = '\0';
		}

		if (line[0] != '\0')
			strcpy(modules[modules_size++], line);
	}
}

static void read_dir(const char* path) {
	DIR*           dir;
	struct dirent* de;

	if ((dir = opendir(path)) == NULL) {
		return;
	}

	char filepath[1024];
	while ((de = readdir(dir)) != NULL) {
		if (de->d_name[0] == '.')
			continue;

		strcpy(filepath, path);
		strcat(filepath, de->d_name);

		read_file(filepath);
	}

	closedir(dir);
}

int main(int argc, char** argv) {
	read_cmdline();

	read_dir("/etc/modules-load.d/");
	read_dir("/run/modules-load.d/");
	read_dir("/usr/lib/modules-load.d/");

	for (int i = 0; i < modules_size; i++) {
		printf("%s\n", modules[i]);
	}

	char* args[modules_size + argc - 1 + 2 + 1];
	int   argi = 0;

	args[argi++] = "modprobe";
	args[argi++] = "-ab";

	for (int i = 1; i < argc; i++) {
		args[argi++] = argv[i];
	}

	for (int i = 0; i < modules_size; i++) {
		args[argi++] = modules[i];
	}

	args[argi++] = NULL;

	execvp("modprobe", args);

	print_error("cannot exec modprobe: %s");
	return 1;
}
