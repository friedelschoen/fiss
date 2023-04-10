#include "config.h"
#include "service.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


void service_store(service_t* s, uint8_t* buffer) {
	buffer[0]  = (s->state);
	buffer[1]  = (s->pid >> 0) & 0xff;
	buffer[2]  = (s->pid >> 8) & 0xff;
	buffer[3]  = (s->pid >> 16) & 0xff;
	buffer[4]  = (s->pid >> 24) & 0xff;
	buffer[5]  = (s->status_change >> 0) & 0xff;
	buffer[6]  = (s->status_change >> 8) & 0xff;
	buffer[7]  = (s->status_change >> 16) & 0xff;
	buffer[8]  = (s->status_change >> 24) & 0xff;
	buffer[9]  = (s->status_change >> 32) & 0xff;
	buffer[10] = (s->status_change >> 40) & 0xff;
	buffer[11] = (s->status_change >> 48) & 0xff;
	buffer[12] = (s->status_change >> 56) & 0xff;
	buffer[13] = (s->fail_count);
	buffer[14] = (s->return_code);
	buffer[15] = (s->last_exit << 0) |
	             (s->restart_file << 2) |
	             (s->restart_manual << 4) |
	             (s->is_log_service << 6) |
	             ((s->log_service != NULL) << 7);

	// +--+--+--+--+--+--+--+--+
	// |FS|ZO|LS|AU|PS|DE|SG|RS|
	// +--+--+--+--+--+--+--+--+
	// RS = restart if died
	// SG = is signaled
	// DE = is dead
	// PS = paused
	// AU = autostart
	// LS = has log service
	// ZM = is zombie (cannot die)
	// FS = has finish script

	// status file
	// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	// |    PID    |  FIN_PID  |     STATUS_CHANGE     |RC|FC|FL|
	// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	// PID = pid of the service (0 if inactive)
	// FIN_PID = pid of the finish script (0 if inactive)
	// STATUS_CHANGE = unix timestamp of the last time the service changed
	// RC = last return code
	// FC = count of failture in a row
	// FL = flags
}

void service_load(service_t* s, const uint8_t* buffer) {
	s->state = buffer[0];
	s->pid   = ((uint32_t) buffer[1] << 0) |
	         ((uint32_t) buffer[2] << 8) |
	         ((uint32_t) buffer[3] << 16) |
	         ((uint32_t) buffer[4] << 24);
	s->status_change = ((uint64_t) buffer[5] << 0) |
	                   ((uint64_t) buffer[6] << 8) |
	                   ((uint64_t) buffer[7] << 16) |
	                   ((uint64_t) buffer[8] << 24) |
	                   ((uint64_t) buffer[9] << 32) |
	                   ((uint64_t) buffer[10] << 40) |
	                   ((uint64_t) buffer[11] << 48) |
	                   ((uint64_t) buffer[12] << 56);
	s->fail_count     = buffer[13];
	s->return_code    = buffer[14];
	s->last_exit      = buffer[15] & 0x03;
	s->restart_file   = (buffer[15] >> 2) & 0x03;
	s->restart_manual = (buffer[15] >> 4) & 0x03;
	s->is_log_service = (buffer[15] >> 6) & 0x01;
	s->log_service    = (buffer[15] >> 7) ? (void*) 1 : NULL;
}