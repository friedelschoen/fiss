#include "service.h"


const char* service_status_name(service_t* s) {
	switch (s->state) {
		case STATE_SETUP:
			return "setup";
		case STATE_STARTING:
			return "starting";
		case STATE_ACTIVE_FOREGROUND:
			return "run";
		case STATE_ACTIVE_BACKGROUND:
			return "run-background";
		case STATE_ACTIVE_DUMMY:
			return "run-dummy";
		case STATE_FINISHING:
			return "finishing";
		case STATE_STOPPING:
			return "stopping";
		case STATE_INACTIVE:
			return "down";
		case STATE_DEAD:
			return "dead";
	}
}

void service_encode(service_t* s, service_serial_t* buffer) {
	uint64_t tai = (uint64_t) s->status_change + 4611686018427387914ULL;
	int      state_runit;

	switch (s->state) {
		case STATE_INACTIVE:
		case STATE_DEAD:
			state_runit = 0;
			break;
		case STATE_SETUP:
		case STATE_STARTING:
		case STATE_ACTIVE_DUMMY:
		case STATE_ACTIVE_FOREGROUND:
		case STATE_ACTIVE_BACKGROUND:
		case STATE_STOPPING:
			state_runit = 1;
			break;
		case STATE_FINISHING:
			state_runit = 2;
			break;
	}

	buffer->status_change[0] = (tai >> 56) & 0xff;
	buffer->status_change[1] = (tai >> 48) & 0xff;
	buffer->status_change[2] = (tai >> 40) & 0xff;
	buffer->status_change[3] = (tai >> 32) & 0xff;
	buffer->status_change[4] = (tai >> 24) & 0xff;
	buffer->status_change[5] = (tai >> 16) & 0xff;
	buffer->status_change[6] = (tai >> 8) & 0xff;
	buffer->status_change[7] = (tai >> 0) & 0xff;

	buffer->state       = s->state;
	buffer->return_code = s->return_code;
	buffer->fail_count  = s->fail_count;

	buffer->flags = (s->restart_file << 4) |
	                (s->restart_manual << 2) |
	                (s->last_exit << 0);

	buffer->pid[0] = (s->pid >> 0) & 0xff;
	buffer->pid[1] = (s->pid >> 8) & 0xff;
	buffer->pid[2] = (s->pid >> 16) & 0xff;
	buffer->pid[3] = (s->pid >> 24) & 0xff;

	buffer->paused      = s->paused;
	buffer->restart     = service_need_restart(s);
	buffer->force_down  = s->restart_manual == S_FORCE_DOWN;
	buffer->state_runit = state_runit;
}


void service_decode(service_t* s, const service_serial_t* buffer) {
	uint64_t tai = ((uint64_t) buffer->status_change[0] << 56) |
	               ((uint64_t) buffer->status_change[1] << 48) |
	               ((uint64_t) buffer->status_change[2] << 40) |
	               ((uint64_t) buffer->status_change[3] << 32) |
	               ((uint64_t) buffer->status_change[4] << 24) |
	               ((uint64_t) buffer->status_change[5] << 16) |
	               ((uint64_t) buffer->status_change[6] << 8) |
	               ((uint64_t) buffer->status_change[7]);

	s->status_change = tai - 4611686018427387914ULL;

	s->state          = buffer->state;
	s->return_code    = buffer->return_code;
	s->fail_count     = buffer->fail_count;
	s->restart_file   = (buffer->flags >> 4) & 0x03;
	s->restart_manual = (buffer->flags >> 2) & 0x03;
	s->last_exit      = (buffer->flags >> 0) & 0x03;

	s->pid = (buffer->pid[0] << 0) |
	         (buffer->pid[1] << 8) |
	         (buffer->pid[2] << 16) |
	         (buffer->pid[3] << 24);

	s->paused        = buffer->paused;
	s->restart_final = buffer->restart;
}
