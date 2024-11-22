#ifndef __NOTSTD_PHQ_H__
#define __NOTSTD_PHQ_H__

#include <notstd/core.h>

/* priority heap queue, aka binary heap */

#ifdef PHQ_IMPLEMENTATION
#include <notstd/field.h>
#endif

//desc return a<b
//asc  return b>a

typedef unsigned(*phqIndexGet_f)(void* data);
typedef void(*phqIndexSet_f)(void* data, unsigned index);

typedef struct phq{
	__rdon cmp_f         cmp;      /**< compare function*/
	__rdon phqIndexGet_f iget;
	__rdon phqIndexSet_f iset;
	__rdon void* __rdon * __rdon queue; /**< array of elements*/
}phq_s;

phq_s* phq_ctor(phq_s* p, size_t size, cmp_f cmp, phqIndexGet_f iget, phqIndexSet_f iset);
void phq_dtor(void* ph);

void phq_new_dtor(void* p);

unsigned phq_size(phq_s* p);

void phq_push(phq_s* p, void* data);

void phq_change_priority(phq_s *p, void* data, unsigned cmpPrio);

void phq_remove(phq_s* p, void* data);

void* phq_pop(phq_s* p);

void* phq_peek(phq_s* p);






#endif
