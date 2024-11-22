#ifndef __NOTSTD_RING_BUFFER_H__
#define __NOTSTD_RING_BUFFER_H__

#include <notstd/core.h>

#ifdef RINGBUFFER_IMPLEMENTATION
#include <notstd/field.h>
#endif

typedef struct rbuffer{
	__prv8 void* buffer;
	__prv8 __atomic unsigned r;
	__prv8 __atomic unsigned w;
	__rdon unsigned size;
	__prv8 unsigned flags;
}rbuffer_s;

rbuffer_s* rbuffer_ctor(rbuffer_s* rb, void* buffer, unsigned size, unsigned sof);
void rbuffer_dtor(void* mem);

#endif

#ifdef USED_RINGBUFFER_FN
#ifndef __NOTSTD_IMPLEMENTATION_RINGBUFFER_H__
#define __NOTSTD_IMPLEMENTATION_RINGBUFFER_H__
__unsafe_begin
__unsafe_unused_fn
__unsafe_deprecated

__private int rbuffer_empty(rbuffer_s* rb){
	return !(rb->r - rb->w);
}

__private int rbuffer_full(rbuffer_s* rb){
	return rb->w + 1 == rb->r;
}

__private unsigned rbuffer_pull_claim(rbuffer_s* rb){
	return rb->w - rb->r;
}

__private void* rbuffer_pull_request(rbuffer_s* rb){
	iassert( rbuffer_pull_claim(rb) );
	return mem_addressing(rb->buffer, FAST_MOD_POW_TWO(rb->r, rb->size));
}

__private void rbuffer_pull_commit(rbuffer_s* rb){
	++rb->r;
}

__private unsigned rbuffer_pull_claim(rbuffer_s* rb){
	return (rb->size - (rb->w - rb->r)) - 1;
}

__private void* rbuffer_push_request(rbuffer_s* rb){
	iassert( rbuffer_push_claim(rb) );
	return mem_addressing((void*)rb->buffer, FAST_MOD_POW_TWO(rb->w, rb->size));
}

__private void rbuffer_push_commit(rbuffer_s* rb){
	++rb->w;
}

__unsafe_end
#endif
#endif
