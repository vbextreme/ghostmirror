#ifndef __NOTSTD_RHHASH_H__
#define __NOTSTD_RHHASH_H__

/*** Robin Hood Hash ***/

#include <notstd/hashalg.h>

#ifdef RHHASH_IMPLEMENTATION
#include <notstd/field.h>
#endif

typedef uint64_t(*rhhash_f)(const void* name, size_t len);

typedef struct rhElement{
	__rdwr void*    data;     /**< user data*/
	__rdon uint64_t hash;     /**< hash */
	__rdon uint32_t len;      /**< len of key*/
	__prv8 uint32_t distance; /**< distance from hash*/
	__rdon char     key[];    /**< flexible key*/
}rhElement_s;

typedef struct rhhash{
	__rdon rhElement_s* __rdon table;  /**< hash table*/
	__rdon unsigned pmin;        /**< percentage elements of free bucket*/
	__rdon unsigned min;         /**< min elements of free bucket*/
	__rdon unsigned maxdistance; /**< max distance from hash*/
	__rdon unsigned keySize;     /**< key size*/
	__rdon unsigned size;
	__rdon rhhash_f hashing;     /**< function calcolate hash*/
}rhhash_s;

typedef uint64_t(*rhhash_f)(const void* name, size_t len);

/************/
/* rhhash.c */
/************/

rhhash_s* rhhash_ctor(rhhash_s* rbh, unsigned size, unsigned min, unsigned keysize, rhhash_f hashing);

void rhhash_dtor(void* rbh);

int rhhash_addh(rhhash_s* rbh, uint64_t hash, const void* key, size_t len, void* data);

int rhhash_add(rhhash_s* rbh, const void* key, size_t len, void* data);

long rhhash_find_bucket(rhhash_s* rbh, uint64_t hash, const void* key, size_t len);

unsigned rhhash_bucket_next(rhhash_s* rbh, unsigned bucket);

int rhhash_addu(rhhash_s* rbh, const void* key, size_t len, void* data);

rhElement_s* rhhash_findh(rhhash_s* rbh, uint64_t hash, const void* key, size_t len);

rhElement_s* rhhash_find(rhhash_s* rbh, const void* key, size_t len);

int rhhash_removeh(rhhash_s* rbh, uint64_t hash, const void* key, size_t len);

int rhhash_remove(rhhash_s* ht, const void* key, size_t len);

unsigned rhhash_bucket_used(rhhash_s* rbh);

unsigned rhhash_collision(rhhash_s* rbh);


#endif
