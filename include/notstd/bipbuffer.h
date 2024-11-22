#ifndef __NOTSTD_CORE_BIPBUFFER_H__
#define __NOTSTD_CORE_BIPBUFFER_H__

#include <notstd/core.h>

typedef struct bipbuffer bipbuffer_t;

bipbuffer_t* bipbuffer_new(unsigned sof, unsigned max);
int bipbuffer_empty(bipbuffer_t* bb);
int bipbuffer_full(bipbuffer_t* bb);
void bipbuffer_clear(bipbuffer_t* bb);
void* bipbuffer_fill(bipbuffer_t* bb, __out unsigned* count);
unsigned bipbuffer_fill_commit(bipbuffer_t* bb, unsigned count);
void* bipbuffer_read(bipbuffer_t* bb, __out unsigned* count);
unsigned bipbuffer_read_commit(bipbuffer_t* bb, unsigned count);

#endif 
