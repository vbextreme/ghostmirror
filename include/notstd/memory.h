#ifndef __NOTSTD_MEMORY_H__
#define __NOTSTD_MEMORY_H__

#include <stddef.h>
#include <notstd/compiler.h>
#include <notstd/xmacro.h>
#include <notstd/type.h>

//realign to next valid address of pointers, prevent strict aliasing
//int v[32];
//int* pv = v[1];
//char* ch = (char*)pv;
//++ch;
//pv = (int*)ch; //strict aliasing
//aliasing_next(pv); //now pv points to correct address, v[2]
#define aliasing_next(PTR) do{ PTR = (void*)(ROUND_UP(ADDR(PTR), sizeof(PTR[0]))) } while(0)

//realign to previus valid address of pointers, prevent strict aliasing
//aliasing_prev(pv); //now pv points to correct address, v[1]
#define aliasing_prev(PTR) do{ PTR = (void*)(ROUND_DOWN(ADDR(PTR), sizeof(PTR[0]))) } while(0)

//return 0 if not violate strict aliasing, otherwise you can't cast without creash your software
#define aliasing_check(PTR) (ADDR(PTR)%sizeof(PTR[0]))

/************/
/* memory.c */
/************/

//raii attribute for cleanup and free memory allocated with mem_alloc
#define __free __cleanup(mem_free_raii)

#define MANY_0(T,C,arg...) (T*)mem_alloc(sizeof(T), (C), NULL)
#define MANY_1(T,C,arg...) (T*)mem_alloc(sizeof(T), (C), ##arg)
#define MANY(T,C,arg...)   CONCAT_EXPAND(MANY_, VA_COUNT(arg))(T,C, ##arg)

#define NEW(T,arg...)      MANY(T,1, ##arg)

#define OBJ(N,arg...)      N##_ctor(mem_alloc(sizeof(N##_s), 1, N##_dtor), ##arg)

#define DELETE(M)          do{ mem_free(M); (M)=NULL; }while(0)

#define RESIZE(T,M,C)      (T*)mem_realloc((M), (C))

//callback type for cleanup object
typedef void (*mcleanup_f)(void*);

#ifdef MEMORY_IMPLEMENTATION
#include <notstd/field.h>
#else
extern unsigned PAGE_SIZE;
#endif

typedef struct hmem{
	__rdwr mcleanup_f cleanup;
	__rdwr unsigned   len;
	__rdon unsigned   sof;
	__rdon unsigned   refs;
	__prv8 int        lock;
	__rdon unsigned   size;
	__prv8 unsigned   flags;
}hmem_s;

typedef struct mslice{
	void*  base;
	size_t begin;
	size_t end;
}mslice_s;

//is called in notstd_begin(), need call only one time
void mem_begin(void);

hmem_s* mem_header(void* addr);

__malloc void* mem_alloc(unsigned sof, size_t count, mcleanup_f dtor);

void* mem_realloc(void* mem, size_t count);

void* mem_upsize(void* mem, size_t count);

void* mem_upsize_zero(void* mem, size_t count);

void* mem_shrink(void* mem);

void* mem_fit(void* mem);
	
void* mem_delete(void* mem, size_t index, size_t count);

void* mem_widen(void* mem, size_t index, size_t count);

void* mem_insert(void* restrict dst, size_t index, void* restrict src, size_t count);

void* mem_push(void* restrict dst, void* restrict element);

void* mem_pop(void* restrict mem, void* restrict element);

void* mem_qsort(void* mem, cmp_f cmp);

void* mem_bsearch(void* mem, void* search, cmp_f cmp);

void* mem_shuffle(void* mem, size_t begin, size_t end);

void* mem_index(void* mem, long index);

void* mem_borrowed(void* mem);


void mem_free(void* addr);

//not use this, this is used for __free
void mem_free_raii(void* addr);

//lock memory for read, all threads can read but nobody can write
int mem_lock_read(void* addr);
//lock memory for writing, wait all threads stop reading, only one can write
int mem_lock_write(void* addr);
//unlock read/write
int mem_unlock(void* addr);
//used for __munlock
void mem_unlock_raii(void* addr);
#define __munlock __cleanup(mem_unlock_raii)

//wow macro for use lock, exaples:
//int *a = NEW(int);
//mem_acquire_read(a){
//	//now all threads can only reads
//	printf("%d", a);
//}
//mem_acquire_write(a){
//	//wait that nobody threads are reading
//	*a = 1;
//}
#define mem_acquire_read(ADDR) for(int __acquire__ = mem_lock_read(ADDR); __acquire__; __acquire__ = 0, mem_unlock(ADDR) )
#define mem_acquire_write(ADDR) for(int __acquire__ = mem_lock_write(ADDR); __acquire__; __acquire__ = 0, mem_unlock(ADDR) )

//simple check for validation memory exaples you can write
//iassert(mem_check(mem));
//to make sure it was allocated with mem_alloc(sizeof(double), 
int mem_check(void* addr);

//memset(0)
void mem_zero(void* addr);

void* mem_nullterm(void* addr);

__unsafe_begin;
__unsafe_unused_fn;

__private size_t mem_size(void* addr){
	return mem_header(addr)->size - sizeof(hmem_s);
}

__private size_t mem_lenght(void* addr){
	hmem_s* hm = mem_header(addr);
	return (hm->size - sizeof(hmem_s)) / hm->sof;
}

__private void* mem_addressing(void* addr, unsigned long index){
	hmem_s* hm = mem_header(addr);
	return (void*)ADDRTO(addr, hm->sof, index);
}

__private size_t mem_available(void* addr){
	hmem_s* hm = mem_header(addr);
	return ((hm->size - sizeof(hmem_s)) / hm->sof) - hm->len;
}

__unsafe_end;

#define mforeach(M,IT) for(unsigned IT = 0; IT < mem_header(M)->len; ++IT)


/************/
/* extras.c */
/************/

//classic way for swap any value
#define swap(A,B) ({ typeof(A) tmp = A; (A) = (B); (B) = tmp; })

//swap memory region, size is how many memory you need to swap, warning size is not checked, -1 if !a||!b||!sizeA||!sizeB
int memswap(void* restrict a, size_t sizeA, void* restrict b, size_t sizeB);

//call memswap but check size and realloc if need
//mem_swap(&a, 5, &b, 7)
int mem_swap(void* restrict a, size_t sza, void* restrict b, size_t szb);


#endif
