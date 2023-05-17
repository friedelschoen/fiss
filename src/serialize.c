#include "service.h"


void service_store(service_t* s, uint8_t* buffer) {
	buffer[0]  = (s->pid >> 0) & 0xff;
	buffer[1]  = (s->pid >> 8) & 0xff;
	buffer[2]  = (s->pid >> 16) & 0xff;
	buffer[3]  = (s->pid >> 24) & 0xff;
	buffer[4]  = (s->status_change >> 0) & 0xff;
	buffer[5]  = (s->status_change >> 8) & 0xff;
	buffer[6]  = (s->status_change >> 16) & 0xff;
	buffer[7]  = (s->status_change >> 24) & 0xff;
	buffer[8]  = (s->status_change >> 32) & 0xff;
	buffer[9]  = (s->status_change >> 40) & 0xff;
	buffer[10] = (s->status_change >> 48) & 0xff;
	buffer[11] = (s->status_change >> 56) & 0xff;
	buffer[12] = (s->fail_count);
	buffer[13] = (s->return_code);
	buffer[14] = (s->state << 0) |
	             (s->restart_file << 4) |
	             (s->restart_manual << 6);
	buffer[15] = (s->last_exit << 0) |
	             (service_need_restart(s) << 2) |
	             (s->paused << 3) |
	             (s->is_log_service << 4) |
	             ((s->log_service != NULL) << 5);
}

const char* service_store_human(service_t* s) {
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

void service_store_runit(service_t* s, uint8_t* buffer) {
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

	buffer[0]  = (tai >> 56) & 0xff;
	buffer[1]  = (tai >> 48) & 0xff;
	buffer[2]  = (tai >> 40) & 0xff;
	buffer[3]  = (tai >> 32) & 0xff;
	buffer[4]  = (tai >> 24) & 0xff;
	buffer[5]  = (tai >> 16) & 0xff;
	buffer[6]  = (tai >> 8) & 0xff;
	buffer[7]  = (tai >> 0) & 0xff;
	buffer[8]  = 0;    // not implemented
	buffer[9]  = 0;    // not implemented
	buffer[10] = 0;    // not implemented
	buffer[11] = 0;    // not implemented
	buffer[12] = (s->pid >> 0) & 0xff;
	buffer[13] = (s->pid >> 8) & 0xff;
	buffer[14] = (s->pid >> 16) & 0xff;
	buffer[15] = (s->pid >> 24) & 0xff;
	buffer[16] = (s->paused);
	buffer[17] = service_need_restart(s) ? 'u' : 'd';
	buffer[18] = 0;    // not implemented
	buffer[19] = runit_state;
}

void service_load(service_t* s, const uint8_t* buffer) {
	s->pid = ((uint32_t) buffer[0] << 0) |
	         ((uint32_t) buffer[1] << 8) |
	         ((uint32_t) buffer[2] << 16) |
	         ((uint32_t) buffer[3] << 24);
	s->status_change = ((uint64_t) buffer[4] << 0) |
	                   ((uint64_t) buffer[5] << 8) |
	                   ((uint64_t) buffer[6] << 16) |
	                   ((uint64_t) buffer[7] << 24) |
	                   ((uint64_t) buffer[8] << 32) |
	                   ((uint64_t) buffer[9] << 40) |
	                   ((uint64_t) buffer[10] << 48) |
	                   ((uint64_t) buffer[11] << 56);
	s->fail_count     = buffer[12];
	s->return_code    = buffer[13];
	s->state          = (buffer[14] >> 0) & 0x0F;
	s->restart_file   = (buffer[14] >> 4) & 0x03;
	s->restart_manual = (buffer[14] >> 6) & 0x03;
	s->last_exit      = (buffer[15] >> 0) & 0x03;
	s->restart_final  = (buffer[15] >> 2) & 0x01;
	s->paused         = (buffer[15] >> 3) & 0x01;
	s->is_log_service = (buffer[15] >> 4) & 0x01;
	s->log_service    = (buffer[15] >> 5) ? (void*) 1 : (void*) 0;
}
