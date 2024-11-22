#ifndef __NOTSTD_DICT_H__
#define __NOTSTD_DICT_H__

#include <notstd/core.h>
#include <notstd/rbtree.h>


#ifdef DICT_IMPLEMENTION
#include <notstd/field.h>
#endif
typedef enum { DP_NUM, DP_STR } dpKeyType_e;

typedef struct dictPair{
	__rdon dpKeyType_e type;
	union{
		long        lkey;
		const char* skey;
		void*       vkey;
	};
	void* value;
}dictPair_s;

typedef rbtree_s dict_s;

#define dict(D,K) _Generic((K),\
	int          : dictg(D, (dictPair_s){.type = DP_NUM, .vkey = (void*)(uintptr_t)K}),\
	long         : dictg(D, (dictPair_s){.type = DP_NUM, .vkey = (void*)(uintptr_t)K}),\
	unsigned int : dictg(D, (dictPair_s){.type = DP_NUM, .vkey = (void*)(uintptr_t)K}),\
	unsigned long: dictg(D, (dictPair_s){.type = DP_NUM, .vkey = (void*)(uintptr_t)K}),\
	char*        : dictg(D, (dictPair_s){.type = DP_STR, .vkey = (void*)(uintptr_t)K}),\
	const char*  : dictg(D, (dictPair_s){.type = DP_STR, .vkey = (void*)(uintptr_t)K})\
)

#define dictrm(D,K) _Generic((K),\
	int          : dictgrm(D, (dictPair_s){.type = DP_NUM, .vkey = (void*)(uintptr_t)K}),\
	long         : dictgrm(D, (dictPair_s){.type = DP_NUM, .vkey = (void*)(uintptr_t)K}),\
	unsigned int : dictgrm(D, (dictPair_s){.type = DP_NUM, .vkey = (void*)(uintptr_t)K}),\
	unsigned long: dictgrm(D, (dictPair_s){.type = DP_NUM, .vkey = (void*)(uintptr_t)K}),\
	char*        : dictgrm(D, (dictPair_s){.type = DP_STR, .vkey = (void*)(uintptr_t)K}),\
	const char*  : dictgrm(D, (dictPair_s){.type = DP_STR, .vkey = (void*)(uintptr_t)K})\
)

dict_s* dict_ctor(dict_s* t);
void dict_dtor(void* d);

dictPair_s* dictPair_ctor(dictPair_s* dp, long lk, const char* sk, void* v);
void dictPair_dtor(void* mem);

void** dictg(dict_s* d, dictPair_s e);
int dictgrm(dict_s* d, dictPair_s e);


#endif
