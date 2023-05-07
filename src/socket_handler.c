#include "service.h"

#include <stdio.h>
#include <unistd.h>


void service_handle_socket(int client) {
	char command[2] = { 0, 0 };
	char service_name[SV_NAME_MAX];

	read(client, command, sizeof(command));

	printf("command: %c\n", command[0]);

	ssize_t service_len = readstr(client, service_name);

	printf("command: %c-%02x with service '%s'\n", command[0], command[1], service_name);

	int        res     = 0;
	int        res_off = 0;
	service_t* response[128];
	service_t* request[128];

	if (service_len > 0) {
		if (command[0] == S_SWITCH) {
			res = service_handle_command(service_name, command[0], command[1], response + res_off);
		} else {
			int req_size = service_pattern(service_name, request, 128);
			if (req_size == 0) {
				res = -EBADSV;
				goto cleanup;
			}
			for (int i = 0; i < req_size; i++) {
				res = service_handle_command(request[i], command[0], command[1], response + res_off);
				if (res < 0)
					goto cleanup;
				res_off += res;
			}
		}
	} else {
		res = service_handle_command(NULL, command[0], command[1], response);
		if (res < 0)
			goto cleanup;

		res_off = res;
	}


cleanup:
	if (res < 0) {
		res *= -1;
		write(client, &res, 1);
		goto cleanup;
	} else {
		write(client, "", 1);
		uint8_t service_buffer[SV_SERIAL_LEN];

		for (int i = 0; i < res_off; i++) {
			service_store(response[i], service_buffer);
			writestr(client, response[i]->name);
			write(client, service_buffer, sizeof(service_buffer));
		}
		write(client, "", 1);
	}
	close(client);
}