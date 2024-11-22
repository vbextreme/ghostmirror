#ifndef __NOTSTD_HASHALG_H__
#define __NOTSTD_HASHALG_H__

#include <notstd/core.h>

/*************/
/* hashalg.c */
/*************/

uint64_t hash_one_at_a_time(const void *key, size_t len);
uint64_t hash_fasthash(const void* key, size_t len);
uint64_t hash_kr(const void* key, size_t len);
uint64_t hash_sedgewicks(const void* key, size_t len);
uint64_t hash_sobel(const void* key, size_t len);
uint64_t hash_weinberger(const void* key, size_t len);
uint64_t hash_elf(const void* key, size_t len);
uint64_t hash_sdbm(const void* key, size_t len);
uint64_t hash_bernstein(const void* key, size_t len);
uint64_t hash_knuth(const void* key, size_t len);
uint64_t hash_partow(const void* key, size_t len);
uint64_t hash64_splitmix(const void* key, __unused size_t len); //only for key as 64 bit value (no strings)
uint64_t hash_murmur_oaat64(const void* key, const size_t len);
uint64_t hash_murmur_oaat32(const void* key, const size_t len);

#endif
