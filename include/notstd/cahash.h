#ifndef __NOTSTD_CAHASH_H__
#define __NOTSTD_CAHASH_H__

/*** Close Addressing Hash ***/

#include <notstd/hashalg.h>
#include <notstd/list.h>

#ifdef CAHASH_IMPLEMENTATION
#include <notstd/field.h>
#endif

typedef uint64_t(*cahash_f)(const void* name, size_t len);

typedef struct caElement{
	inherit_ld(struct caElement);
	__rdwr void*    data;     /**< user data*/
	__rdon uint64_t hash;     /**< hash */
	__rdon uint32_t len;      /**< len of key*/
	__rdon char     key[];    /**< flexible key*/
}caElement_s;

typedef struct cahash{
	__rdon caElement_s* __rdon * __rdon table;  /**< hash table*/
	__rdon cahash_f     hashing;     /**< function calcolate hash*/
	__rdon unsigned     size;
	__prv8 caElement_s  err;
}cahash_s;


/************/
/* cahash.c */
/************/


cahash_s* cahash_ctor(cahash_s* cah, unsigned size, cahash_f hashing);

void cahash_dtor(void* rbh);

int cahash_addh(cahash_s* cah, uint64_t hash, const void* key, size_t len, void* data);

int cahash_add(cahash_s* cah, const void* key, size_t len, void* data);

caElement_s* cahash_findh(cahash_s* cah, uint64_t hash, const void* key, size_t len);

int cahash_addu(cahash_s* cah, const void* key, size_t len, void* data);

caElement_s* cahash_find(cahash_s* cah, const void* key, size_t len);

int cahash_removeh(cahash_s* cah, uint64_t hash, const void* key, size_t len);

int cahash_remove(cahash_s* cah, const void* key, size_t len);

unsigned cahash_bucket_used(cahash_s* cah);

unsigned cahash_collision(cahash_s* rbh);

#endif
