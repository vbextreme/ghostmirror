#undef DBG_ENABLE

#include <notstd/futex.h>
#include <notstd/debug.h>
#include <notstd/mathmacro.h>
#include <notstd/error.h>
#include <notstd/mth.h>

#include <stdlib.h>
#include <string.h>

#define MEMORY_IMPLEMENTATION
#include <notstd/memory.h>

#define HMEM_FLAG_CHECK    0xF1CA
#define HMEM_CHECK(HM)     (((HM)->flags & 0xFFFF) == HMEM_FLAG_CHECK)
#define HMEM_TO_ADDR(HM)   ((void*)(ADDR(HM)+sizeof(hmem_s)))
#define ADDR_TO_HMEM(A)    ((hmem_s*)(ADDR(A)-sizeof(hmem_s)))

unsigned PAGE_SIZE;

#ifdef OS_LINUX
#include <unistd.h>
#define os_page_size() sysconf(_SC_PAGESIZE);
#else
#define os_page_size() 512
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// lock multiple reader one writer fork https://gist.github.com/smokku/653c469d695d60be4fe8170630ba8205 

#define LOCK_OPEN    1
#define LOCK_WLOCKED 0

__private void lock_ctor(hmem_s* hm){
	hm->lock = LOCK_OPEN;
}

__private void unlock(hmem_s* hm){
	int32_t current, wanted;
	do {
		current = hm->lock;
		if( current == LOCK_OPEN ) return;
		wanted = current == LOCK_WLOCKED ? LOCK_OPEN : current - 1;
	}while( __sync_val_compare_and_swap(&hm->lock, current, wanted) != current );
	futex(&hm->lock, FUTEX_WAKE, 1, NULL, NULL, 0);
}

__private void lock_read(hmem_s* hm){
    int32_t current;
	while( (current = hm->lock) == LOCK_WLOCKED || __sync_val_compare_and_swap(&hm->lock, current, current + 1) != current ){
		while( futex(&hm->lock, FUTEX_WAIT, current, NULL, NULL, 0) != 0 ){
			cpu_relax();
			if (hm->lock >= LOCK_OPEN) break;
		}
	}
}

__private void lock_write(hmem_s* hm){
	unsigned current;
	while( (current = __sync_val_compare_and_swap(&hm->lock, LOCK_OPEN, LOCK_WLOCKED)) != LOCK_OPEN ){
		while( futex(&hm->lock, FUTEX_WAIT, current, NULL, NULL, 0) != 0 ){
			cpu_relax();
			if( hm->lock == LOCK_OPEN ) break;
		}
		if( hm->lock != LOCK_OPEN ){
			futex(&hm->lock, FUTEX_WAKE, 1, NULL, NULL, 0);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// memory manager

void mem_begin(void){
	PAGE_SIZE  = os_page_size();
}

__private inline hmem_s* givehm(void* addr){
	hmem_s* hm = ADDR_TO_HMEM(addr);
	iassert(HMEM_CHECK(hm));
	return hm;
}

hmem_s* mem_header(void* addr){
	return givehm(addr);
}

__malloc void* mem_alloc(unsigned sof, size_t count, mcleanup_f dtor){
	iassert(sof);
	iassert(count);
	size_t size = sof * count + sizeof(hmem_s);
	size = ROUND_UP(size, sizeof(uintptr_t));
	hmem_s* hm  = malloc(size);
	if( !hm ) die("on malloc: %m");
	hm->refs    = 1;
	hm->flags   = HMEM_FLAG_CHECK;
	hm->size    = size;
	hm->cleanup = dtor;
	hm->len     = 0;
	hm->sof     = sof;
	lock_ctor(hm);
	void* ret = HMEM_TO_ADDR(hm);
	iassert( ADDR(ret) % sizeof(uintptr_t) == 0 );
	dbg_info("mem addr: %p header: %p", ret, hm);
	return ret;
}

void* mem_realloc(void* mem, size_t count){
	hmem_s* hm = givehm(mem);
	size_t size = sizeof(hmem_s) + count * hm->sof;
	size  = ROUND_UP(size, sizeof(uintptr_t));
	dbg_info("realloc to %lu", size);

	hm = realloc(hm, size);
	if( !hm ) die("on realloc: %m");
	hm->size = size;

	void* ret = HMEM_TO_ADDR(hm);
	iassert( ADDR(ret) % sizeof(uintptr_t) == 0 );
	return ret;
}

void* mem_upsize(void* mem, size_t count){
	hmem_s* hm = givehm(mem);
	size_t lenght = (hm->size - sizeof(hmem_s)) / hm->sof;
	dbg_info("lenght: %lu count: %lu len: %u", lenght, count, hm->len);
	if( hm->len + count > lenght  ){
		lenght = (lenght + count) * 2;
		mem = mem_realloc(mem, lenght);
	}
	return mem;
}

void* mem_upsize_zero(void* mem, size_t count){
	hmem_s* hm = givehm(mem);
	size_t lenght = (hm->size - sizeof(hmem_s)) / hm->sof;
	if( hm->len + count > lenght  ){
		size_t newlen = (lenght + count) * 2;
		mem = mem_realloc(mem, newlen);
		hmem_s* hm = givehm(mem);
		memset((void*)ADDRTO(mem, hm->sof, lenght), 0, (newlen-lenght) * hm->sof);
	}
	return mem;
}

void* mem_shrink(void* mem){
	hmem_s* hm = givehm(mem);
	size_t lenght = (hm->size - sizeof(hmem_s)) / hm->sof;
	if( lenght / 4 > hm->len ){
		lenght /= 2;
		mem = mem_realloc(mem, lenght);
	}
	return mem;
}

void* mem_fit(void* mem){
	return mem_realloc(mem, givehm(mem)->len);
}

void* mem_delete(void* mem, size_t index, size_t count ){
	if( !count ){
		errno = EINVAL;
		return mem;
	}
	
	hmem_s* hm = givehm(mem);

	if( index >= hm->len ){
		errno = EINVAL;
		return mem;
	}
	
	if( index + count >= hm->len ){
		hm->len = index;
		return mem;
	}
	
	void*  dst  = (void*)ADDRTO(mem, hm->sof, index);
	void*  src  = (void*)ADDRTO(mem, hm->sof, (index+count));
	size_t size = (hm->len - (index+count)) * hm->sof;
	memmove(dst, src, size);
	hm->len -= count;
	
	return mem;
}

void* mem_widen(void* mem, size_t index, size_t count){
	if( !count ){
		errno = EINVAL;
		return mem;
	}
	mem = mem_upsize(mem, count);
	hmem_s* hm = givehm(mem);
	
	if( index > hm->len ){
		errno = EINVAL;
		return mem;
	}
	
	if( index == hm->len ){
		hm->len = index + count;
		return mem;
	}
	
	void*  src  = (void*)ADDRTO(mem, hm->sof, index);
	void*  dst  = (void*)ADDRTO(mem, hm->sof, (index+count));
	size_t size = (hm->len - (index+count)) * hm->sof;
	memmove(dst, src, size);
	hm->len += count;
	
	return mem;
}

void* mem_insert(void* restrict dst, size_t index, void* restrict src, size_t count){
	errno = 0;
	dst = mem_widen(dst, index, count);
	if( errno ) return dst;
	
	const unsigned sof = givehm(dst)->sof;
	void* draw = (void*)ADDRTO(dst, sof, index);
	memcpy(draw, src, count * sof);
	
	return dst;
}

void* mem_push(void* restrict dst, void* restrict element){
	dst = mem_upsize(dst, 1);
	hmem_s* hm = givehm(dst);
	void* draw = (void*)ADDRTO(dst, hm->sof, hm->len);
	memcpy(draw, element, hm->sof);
	++hm->len;
	return dst;
}

void* mem_pop(void* restrict mem, void* restrict element){
	hmem_s* hm = givehm(mem);
	if( !hm->len ) return NULL;
	
	if( element ){
		void* draw = (void*)ADDRTO(mem, hm->sof, hm->len-1);
		memcpy(element, draw, hm->sof);
	}
	--hm->len;
	
	return element;
}

void* mem_qsort(void* mem, cmp_f cmp){
	hmem_s* hm = givehm(mem);
	qsort(mem, hm->len, hm->sof, cmp);
	return mem;
}

void* mem_bsearch(void* mem, void* search, cmp_f cmp){
	hmem_s* hm = givehm(mem);
	return bsearch(search, mem, hm->len, hm->sof, cmp);
}

void* mem_shuffle(void* mem, size_t begin, size_t end){
	hmem_s* hm = givehm(mem);
	if( end == 0 && hm->len ) end = hm->len-1;
	if( end == 0 ) return mem;
	
	const size_t count = (end - begin) + 1;
	for( size_t i = begin; i <= end; ++i ){
		size_t j = begin + mth_random(count);
		if( j != i ){
			void* a = (void*)ADDRTO(mem, hm->sof, i);
			void* b = (void*)ADDRTO(mem, hm->sof, j);
			memswap(a , hm->sof, b, hm->sof);
		}
	}
	return mem;
}

void* mem_index(void* mem, long index){
	hmem_s* hm = givehm(mem);
	if( labs(index) > (long)hm->len ){
		if( index < 0 ) index = 0;
		else            index = hm->len-1;
	}
	
	if( index < 0 )	index = hm->len - index;
	return (void*)ADDRTO(mem, hm->sof, index);
}

void* mem_borrowed(void* mem){
	if( mem ) ++givehm(mem)->refs;
	return mem;
}

void mem_free(void* addr){
	dbg_info("free addr: %p", addr);
	if( !addr ) return;
	hmem_s* hm = ADDR_TO_HMEM(addr);
	iassert( HMEM_CHECK(hm) );
	iassert( hm->refs );
	if( --hm->refs ) return;
	if( hm->cleanup ) hm->cleanup(HMEM_TO_ADDR(hm));
	free(hm);
}

void mem_free_raii(void* addr){
	mem_free(*(void**)addr);
}

int mem_lock_read(void* addr){
	lock_read(givehm(addr));
	return 1;
}

int mem_lock_write(void* addr){
	lock_write(givehm(addr));
	return 1;
}

int mem_unlock(void* addr){
	unlock(givehm(addr));
	return 0;
}

void mem_unlock_raii(void* addr){
	mem_unlock(*(void**)addr);
}

int mem_check(void* addr){
	hmem_s* hm = ADDR_TO_HMEM(addr);
	return HMEM_CHECK(hm);
}

void mem_zero(void* addr){
	size_t size = mem_size(addr);
	memset(addr, 0, size);
}

void* mem_nullterm(void* addr){
	hmem_s* hm = givehm(addr);
	if( hm->len + 1 > hm->size - sizeof(hmem_s) ){
		addr = mem_realloc(addr, hm->len+1);
		hm = givehm(addr);
	}
	((char*)addr)[hm->len] = 0;
	return addr;
}

