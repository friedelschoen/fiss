#include "service.h"


const char* service_status_name(service_t* s) {
	switch (s->state) {
		case STATE_SETUP:
			return "setup";
		case STATE_STARTING:
			return "starting";
		case STATE_ACTIVE_DUMMY:
		case STATE_ACTIVE_FOREGROUND:
		case STATE_ACTIVE_BACKGROUND:
		case STATE_ACTIVE_PID:
			return "run";
		case STATE_STOPPING:
			return "stopping";
		case STATE_FINISHING:
			return "finishing";
		case STATE_INACTIVE:
			return "down";
		case STATE_DEAD:
			return "dead";
	}
}

void service_encode(service_t* s, service_serial_t* buffer) {
	buffer->pid[0] = (s->pid >> 0) & 0xff;
	buffer->pid[1] = (s->pid >> 8) & 0xff;
	buffer->pid[2] = (s->pid >> 16) & 0xff;
	buffer->pid[3] = (s->pid >> 24) & 0xff;

	buffer->status_change[0] = (s->status_change >> 0) & 0xff;
	buffer->status_change[1] = (s->status_change >> 8) & 0xff;
	buffer->status_change[2] = (s->status_change >> 16) & 0xff;
	buffer->status_change[3] = (s->status_change >> 24) & 0xff;
	buffer->status_change[4] = (s->status_change >> 32) & 0xff;
	buffer->status_change[5] = (s->status_change >> 40) & 0xff;
	buffer->status_change[6] = (s->status_change >> 48) & 0xff;
	buffer->status_change[7] = (s->status_change >> 56) & 0xff;

	buffer->failcount[0]   = (s->fail_count);
	buffer->return_code[0] = (s->return_code);

	buffer->flags[0] = (s->state << 4) |
	                   (s->restart_file << 2) |
	                   (s->restart_manual << 0);
	buffer->flags[1] = (s->last_exit << 6) |
	                   (service_need_restart(s) << 5) |
	                   (s->paused << 4) |
	                   (s->is_log_service << 3) |
	                   ((s->log_service != NULL) << 2);
}

void service_encode_runit(service_t* s, service_serial_runit_t* buffer) {
	uint64_t tai = (uint64_t) s->status_change + 4611686018427387914ULL;
	int      runit_state;
	switch (s->state) {
		case STATE_INACTIVE:
		case STATE_DEAD:
			runit_state = 0;
			break;
		case STATE_SETUP:
		case STATE_STARTING:
		case STATE_ACTIVE_DUMMY:
		case STATE_ACTIVE_FOREGROUND:
		case STATE_ACTIVE_BACKGROUND:
		case STATE_ACTIVE_PID:
			runit_state = 1;
			break;
		case STATE_FINISHING:
		case STATE_STOPPING:
			runit_state = 2;
			break;
	}

	buffer->status_change[0]      = (tai >> 56) & 0xff;
	buffer->status_change[1]      = (tai >> 48) & 0xff;
	buffer->status_change[2]      = (tai >> 40) & 0xff;
	buffer->status_change[3]      = (tai >> 32) & 0xff;
	buffer->status_change[4]      = (tai >> 24) & 0xff;
	buffer->status_change[5]      = (tai >> 16) & 0xff;
	buffer->status_change[6]      = (tai >> 8) & 0xff;
	buffer->status_change[7]      = (tai >> 0) & 0xff;
	buffer->status_change_nsec[0] = 0;    // not implemented
	buffer->status_change_nsec[1] = 0;    // not implemented
	buffer->status_change_nsec[2] = 0;    // not implemented
	buffer->status_change_nsec[3] = 0;    // not implemented
	buffer->pid[0]                = (s->pid >> 0) & 0xff;
	buffer->pid[1]                = (s->pid >> 8) & 0xff;
	buffer->pid[2]                = (s->pid >> 16) & 0xff;
	buffer->pid[3]                = (s->pid >> 24) & 0xff;
	buffer->paused[0]             = (s->paused);
	buffer->wants_up[0]           = service_need_restart(s) ? 'u' : 'd';
	buffer->terminated[0]         = 0;    // not implemented
	buffer->state[0]              = runit_state;
}

void service_decode(service_t* s, const service_serial_t* buffer) {
	s->pid = (buffer->pid[3] << 24) |
	         (buffer->pid[2] << 16) |
	         (buffer->pid[1] << 8) |
	         (buffer->pid[0]);

	s->status_change = ((uint64_t) buffer->status_change[7] << 56) |
	                   ((uint64_t) buffer->status_change[6] << 48) |
	                   ((uint64_t) buffer->status_change[5] << 40) |
	                   ((uint64_t) buffer->status_change[4] << 32) |
	                   ((uint64_t) buffer->status_change[3] << 24) |
	                   ((uint64_t) buffer->status_change[2] << 16) |
	                   ((uint64_t) buffer->status_change[1] << 8) |
	                   ((uint64_t) buffer->status_change[0]);

	s->fail_count  = buffer->failcount[0];
	s->return_code = buffer->return_code[0];

	s->state          = (buffer->flags[0] >> 6) & 0x0F;
	s->restart_file   = (buffer->flags[0] >> 4) & 0x03;
	s->restart_manual = (buffer->flags[0] >> 2) & 0x03;

	s->last_exit      = (buffer->flags[1] >> 6) & 0x03;
	s->restart_file   = (buffer->flags[1] >> 5) & 0x01;
	s->paused         = (buffer->flags[1] >> 4) & 0x01;
	s->is_log_service = (buffer->flags[1] >> 3) & 0x01;
	s->log_service    = (buffer->flags[1] >> 2) & 0x01 ? (void*) 1 : (void*) 0;
}
