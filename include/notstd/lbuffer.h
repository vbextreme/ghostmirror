#ifndef __NOTSTD_CORE_LBUFFER_H__
#define __NOTSTD_CORE_LBUFFER_H__

#include <notstd/core.h>

typedef struct lbuffer lbuffer_t;

lbuffer_t* lbuffer_new(unsigned size);
unsigned lbuffer_current_fill_available(lbuffer_t* lb);
void* lbuffer_fill(lbuffer_t* lb, __out unsigned* size);
unsigned lbuffer_fill_commit(lbuffer_t* lb, unsigned size);
void lbuffer_fill_finished(lbuffer_t* lb);
void* lbuffer_read(lbuffer_t* lb, __out unsigned* size);
unsigned lbuffer_read_commit(lbuffer_t* lb, unsigned size);
void lbuffer_ready_finished(lbuffer_t* lb);
size_t lbuffer_available(lbuffer_t* lb);



#endif
