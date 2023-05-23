#include "service.h"
#include "util.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>


const char* command_error[] = {
	[0]       = "success",
	[EBADCMD] = "command not found",
	[ENOSV]   = "service required",
	[EBADSV]  = "no matching services",
	[EBEXT]   = "invalid extra"
};

int service_send_command(char command, char extra, const char* service, service_t* response, int response_max) {
	char request[2] = { command, extra };

	int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sockfd == -1) {
		print_error("error: cannot connect to socket: %s\n");
		exit(1);
	}

	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, sizeof(addr.sun_path), SV_CONTROL_SOCKET, runlevel);

	int ret;
	if ((ret = connect(sockfd, (struct sockaddr*) &addr, sizeof(addr))) == -1) {
		print_error("error: cannot connect to %s: %s\n", addr.sun_path);
		exit(EXIT_FAILURE);
	}

	write(sockfd, request, sizeof(request));
	writestr(sockfd, service);

	int res;
	read(sockfd, &res, 1);

	service_serial_t service_buffer;

	if (res == 0) {
		if (response) {
			while (res < response_max && readstr(sockfd, response[res].name) > 1) {
				read(sockfd, &service_buffer, sizeof(service_buffer));
				service_decode(&response[res], &service_buffer);
				res++;
			}
		}
	} else {
		res *= -1;
	}

	close(sockfd);
	return res;
}