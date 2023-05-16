#include "service.h"


void service_store(service_t* s, uint8_t* buffer) {
	// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	// |    DIR    |    PID    |     STATUS CHANGE     |FC|RC|FLAGS|
	// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	// ST = status
	// DIR = file descriptor to the running directory
	// PID = pid of the current instance (dependening on state)
	// STATUS CHANGE = unix timestamp of last update (why tai tho?)
	// FC = fail count
	// RC = last return code (0 if not exitted yet)

	// +--+--+--+--+--+--+--+--*--+--+--+--+--+--+--+--+
	// |   STATE   |RSFIL|RSMAN|LSTEX|RD|PS|LS|HL| --- |
	// +--+--+--+--+--+--+--+--*--+--+--+--+--+--+--+--+
	// RSFIL = restart file (up-<runlevel>; 0 = down, 2 = once, 3 = restart)
	// RSMAN = restart manual override (0 = down, 1 = force down, 2 = once, 3 = restart)
	// RD = absolute restart needed, combining above + dependencies
	// LSTEX = last exit (0 = never exitted, 1 = normally, 2 = signaled)
	// PS = paused
	// LS = is log service
	// HS = has log service (in struct is pointer but stored as (void*) 1 or (void*) 0)

	buffer[0]  = (s->dir >> 0) & 0xff;
	buffer[1]  = (s->dir >> 8) & 0xff;
	buffer[2]  = (s->dir >> 16) & 0xff;
	buffer[3]  = (s->dir >> 24) & 0xff;
	buffer[4]  = (s->pid >> 0) & 0xff;
	buffer[5]  = (s->pid >> 8) & 0xff;
	buffer[6]  = (s->pid >> 16) & 0xff;
	buffer[7]  = (s->pid >> 24) & 0xff;
	buffer[8]  = (s->status_change >> 0) & 0xff;
	buffer[9]  = (s->status_change >> 8) & 0xff;
	buffer[10] = (s->status_change >> 16) & 0xff;
	buffer[11] = (s->status_change >> 24) & 0xff;
	buffer[12] = (s->status_change >> 32) & 0xff;
	buffer[13] = (s->status_change >> 40) & 0xff;
	buffer[14] = (s->status_change >> 48) & 0xff;
	buffer[15] = (s->status_change >> 56) & 0xff;
	buffer[16] = (s->fail_count);
	buffer[17] = (s->return_code);
	buffer[18] = (s->state << 0) |
	             (s->restart_file << 4) |
	             (s->restart_manual << 6);
	buffer[19] = (s->last_exit << 0) |
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
	// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	// |      TAI SECONDS      | TAIA NANO |    PID    |PS|WU|TR|ST|
	// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	// TAI SECONDS = unix seconds + 4611686018427387914ULL
	// TAIA NANO = unix nanoseconds (nulled-out as fiss don't store them)
	// PID = current pid
	// PS = is paused (int boolean)
	// WU = wants up ('u' if want up, 'd' if want down)
	// TR = was terminated (int boolean)
	// ST = state (0 is down, 1 is running, 2 is finishing)

	uint64_t tai = s->status_change + 4611686018427387914ULL;
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

	buffer[0]  = (tai >> 0) & 0xff;
	buffer[1]  = (tai >> 8) & 0xff;
	buffer[2]  = (tai >> 16) & 0xff;
	buffer[3]  = (tai >> 24) & 0xff;
	buffer[4]  = (tai >> 32) & 0xff;
	buffer[5]  = (tai >> 40) & 0xff;
	buffer[6]  = (tai >> 48) & 0xff;
	buffer[7]  = (tai >> 56) & 0xff;
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
	s->dir = ((uint32_t) buffer[0] << 0) |
	         ((uint32_t) buffer[1] << 8) |
	         ((uint32_t) buffer[2] << 16) |
	         ((uint32_t) buffer[3] << 24);
	s->pid = ((uint32_t) buffer[4] << 0) |
	         ((uint32_t) buffer[5] << 8) |
	         ((uint32_t) buffer[6] << 16) |
	         ((uint32_t) buffer[7] << 24);
	s->status_change = ((uint64_t) buffer[8] << 0) |
	                   ((uint64_t) buffer[9] << 8) |
	                   ((uint64_t) buffer[10] << 16) |
	                   ((uint64_t) buffer[11] << 24) |
	                   ((uint64_t) buffer[12] << 32) |
	                   ((uint64_t) buffer[13] << 40) |
	                   ((uint64_t) buffer[14] << 48) |
	                   ((uint64_t) buffer[15] << 56);
	s->fail_count     = buffer[16];
	s->return_code    = buffer[17];
	s->state          = (buffer[18] >> 0) & 0x0F;
	s->restart_file   = (buffer[18] >> 4) & 0x03;
	s->restart_manual = (buffer[18] >> 6) & 0x03;
	s->last_exit      = (buffer[19] >> 0) & 0x03;
	s->restart_final  = (buffer[19] >> 2) & 0x01;
	s->paused         = (buffer[19] >> 3) & 0x01;
	s->is_log_service = (buffer[19] >> 4) & 0x01;
	s->log_service    = (buffer[19] >> 5) ? (void*) 1 : (void*) 0;
}
