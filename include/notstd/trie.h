#ifndef __NOTSTD_TRIE_H__
#define __NOTSTD_TRIE_H__

#include <notstd/core.h>

#ifdef TRIE_IMPLEMENTATION
#include <notstd/field.h>
#endif

typedef struct trieE{
	__rdon char*         str;
	__rdon unsigned      len;
	__rdon struct trieN* __rdon next;
	__rdwr void*         data;
}trieE_s;

typedef struct trieN{
	__rdon trieE_s* __rdon ve;
}trieN_s;

typedef struct trie{
	__rdon trieN_s* __rdon root;
	__rdon unsigned count;
}trie_s;

trie_s* trie_ctor(trie_s* tr);

void trie_dtor(void* t);

int trie_insert(trie_s* tr, const char* str, unsigned len, void* data);
void* trie_find(trie_s* tr, const char* str, unsigned len);
int trie_remove(trie_s* tr,  const char* str, unsigned len);
void trie_dump(trie_s* tr);

#endif
