#include "service.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>


string command_error[] = {
	[0]       = "success",
	[EBADCMD] = "command not found",
	[ENOSV]   = "service required",
	[EBADSV]  = "no matching services",
	[EBEXT]   = "invalid extra"
};

string command_string[] = {
	(void*) S_START, "start",      // start if not running and restart if failed
	(void*) S_START, "up",         // start if not running and restart if failed
	(void*) S_STOP, "stop",        // stop if running and not restart if failed
	(void*) S_STOP, "down",        // stop if running and not restart if failed
	(void*) S_SEND, "send",        // + signal | send signal to service
	(void*) S_SEND, "kill",        // + signal | send signal to service
	(void*) S_PAUSE, "pause",      // pause service (send SIGSTOP)
	(void*) S_RESUME, "resume",    // unpause service (send SIGCONT)
	(void*) S_REVIVE, "revive",    // revive died service
	(void*) S_UPDATE, "update",    // force update info // todo
	(void*) S_STATUS, "status",    // get status of all services
	(void*) S_EXIT, "exit",        // exit
	(void*) S_CHLEVEL, "chlevel",
	0, 0
};


int service_command(char command, char extra, string service, service_t* response, int response_max) {
	char request[2] = { command, extra };

	int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sockfd == -1) {
		print_error("cannot connect to socket");
		exit(1);
	}

	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, sizeof(addr.sun_path), SV_CONTROL_SOCKET, runlevel);

	int ret = connect(sockfd, (struct sockaddr*) &addr, sizeof(addr));
	if (ret == -1) {
		print_error("cannot connect to %s", addr.sun_path);
		exit(EXIT_FAILURE);
	}

	write(sockfd, request, sizeof(request));
	writestr(sockfd, service);

	int res;
	read(sockfd, &res, 1);

	uint8_t service_buffer[19];

	if (res == 0) {
		if (response) {
			while (res < response_max && readstr(sockfd, response[res].name) > 1) {
				read(sockfd, service_buffer, sizeof(service_buffer));
				service_load(&response[res], service_buffer);
				//			print_service(&s);
				res++;
			}
		}
	} else {
		res *= -1;
	}

	close(sockfd);
	return res;
}

char service_get_command(string command) {
	char cmd_abbr = '\0';
	for (string* cn = command_string; cn[1] != NULL; cn += 2) {
		if (streq(command, cn[1])) {
			cmd_abbr = (size_t) cn[0];
			break;
		}
	}
	return cmd_abbr;
}