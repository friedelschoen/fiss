#include "service.h"


void service_encode(struct service* s, struct service_serial* buffer) {
	uint64_t tai = (uint64_t) s->state_change + 4611686018427387914llu;

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

	buffer->flags = ((s->stop_timeout != 0) << 4) |
	                ((s->restart == S_ONCE) << 3) |
	                ((s->restart == S_RESTART) << 2) |
	                (s->last_exit << 0);

	buffer->pid[0] = (s->pid >> 0) & 0xff;
	buffer->pid[1] = (s->pid >> 8) & 0xff;
	buffer->pid[2] = (s->pid >> 16) & 0xff;
	buffer->pid[3] = (s->pid >> 24) & 0xff;

	buffer->paused     = s->paused;
	buffer->restart    = service_need_restart(s) ? 'u' : 'd';
	buffer->force_down = 0;

	switch (s->state) {
		case STATE_INACTIVE:
		case STATE_ERROR:
		case STATE_DONE:
			buffer->state_runit = 0;    // inactive
			break;
		case STATE_SETUP:
		case STATE_STARTING:
		case STATE_ACTIVE_DUMMY:
		case STATE_ACTIVE_FOREGROUND:
		case STATE_ACTIVE_BACKGROUND:
		case STATE_STOPPING:
			buffer->state_runit = 1;    // running
			break;
		case STATE_FINISHING:
			buffer->state_runit = 2;    // finishing
			break;
	}
}
