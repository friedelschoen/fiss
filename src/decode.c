#include "service.h"


void service_decode(service_t* s, const void* buffer_ptr) {
	const struct service_serial* buffer = buffer_ptr;

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
	s->is_dependency  = (buffer->flags >> 6) & 0x01;
	s->restart_file   = (buffer->flags >> 4) & 0x03;
	s->restart_manual = (buffer->flags >> 2) & 0x03;
	s->last_exit      = (buffer->flags >> 0) & 0x03;

	s->pid = (buffer->pid[0] << 0) |
	         (buffer->pid[1] << 8) |
	         (buffer->pid[2] << 16) |
	         (buffer->pid[3] << 24);

	s->paused        = buffer->paused;
	s->restart_final = buffer->restart == 'u';
}
