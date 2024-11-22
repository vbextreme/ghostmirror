#ifndef __NOTSTD_RBTREE_H__
#define __NOTSTD_RBTREE_H__

#include <notstd/core.h>

#ifdef RBTREE_IMPLEMENTATION
#include <notstd/field.h>
#endif

#define RBT_BLACK   0
#define RBT_RED     1
#define RBT_RAINBOW 2

typedef struct rbtNode{
	__rdon struct rbtNode* __rdon parent;
	__rdon struct rbtNode* __rdon left;
	__rdon struct rbtNode* __rdon right;
	__rdwr void* data;
	__rdon int color;
}rbtNode_s;

typedef struct rbtree{
	__rdon rbtNode_s* root;
	cmp_f cmp;
	__rdon size_t count;
}rbtree_s;

typedef struct rbtreeit{
	__rdon unsigned count;
	__rdon rbtNode_s* __rdon  cur;
	__rdon rbtNode_s* __rdon* __rdon stk;
}rbtreeit_s;

rbtNode_s* rbtNode_ctor(rbtNode_s* n, void* data);

void rbtNode_dtor(void* node);

rbtNode_s* rbtree_insert(rbtree_s* rbt, rbtNode_s* page);
rbtNode_s* rbtree_remove(rbtree_s* rbt, rbtNode_s* p);
rbtNode_s* rbtree_find(rbtree_s* rbt, const void* key);
rbtNode_s* rbtree_find_best(rbtree_s* rbt, const void* key);

void* rbtree_iterate_inorder(rbtreeit_s* it);
void* rbtree_iterate_preorder(rbtreeit_s* it);
rbtreeit_s* rbtreeit_ctor(rbtreeit_s* it, rbtree_s* t, unsigned count);
void rbtreeit_dtor(void* i);

rbtree_s* rbtree_ctor(rbtree_s* t, cmp_f fn);
void rbtree_dtor(void* tree);

#endif
