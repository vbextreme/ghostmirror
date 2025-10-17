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
// memory manager


void mem_begin(void){
	PAGE_SIZE  = os_page_size();
}

void* m_alloc(unsigned sof, size_t count, mcleanup_f dtor, unsigned flags){
	if( sof < 1 || sof > UINT16_MAX ) die("unsupported sizeof %u", sof);
	if( !count ) die("invalid alloc with 0 elements");
	const unsigned aligned = flags & M_FLAG_ALIGN32 ? 16: 0;
	const size_t size = sof * count + sizeof(hmem_s) + aligned;
	void* raw = malloc(size);
	if( !raw ) die("malloc: %m");
	hmem_s* hm = raw;
	if( aligned ){
		if( (uintptr_t)raw % 32 ){
			iassert(raw%16 == 0);
			hm = (void*)((uintptr_t)raw+aligned);
			iassert(hm%32 == 0);
			hm->flags = M_FLAG_CHECK | M_FLAG_OFF16 | M_FLAG_ALIGN32;
		}
		else{
			hm->flags = M_FLAG_CHECK | M_FLAG_ALIGN32;
		}
	}
	else{
		hm->flags   = M_FLAG_CHECK;
	}
	hm->refs    = 1;
	hm->count   = count;
	hm->cleanup = dtor;
	hm->len     = 0;
	hm->sof     = sof;
	void* ret   = hm_toaddr(hm);
	iassert( ADDR(ret) % 16 == 0 );
	dbg_info("mem addr: %p header: %p", ret, hm);
	return ret;
}

void* m_realloc(void* mem, size_t count){
	hmem_s* hm = m_header(mem);
	if( hm->count == count ) return mem;
	if( hm->refs > 1 ) die("unsafe realloc shared ptr");
	const unsigned aligned    = hm->flags & M_FLAG_ALIGN32 ? 16: 0;
	const unsigned haveoffset = hm->flags & M_FLAG_OFF16;
	const size_t size = sizeof(hmem_s) + count * hm->sof + aligned;
	void* raw = (void*)((uintptr_t)hm - aligned);
	hm->flags &= ~M_FLAG_OFF16;
	raw = realloc(raw, size);
	if( !raw ) die("realloc: %m");
	hm = raw;
	if( aligned ){
		iassert(raw%16 == 0);
		const unsigned needoffset = (uintptr_t)raw%32;
		if( haveoffset ){
			if( !needoffset ){
				memmove(raw, raw+aligned, size-aligned);
			}
			hm = (void*)((uintptr_t)raw);
		}
		else if( needoffset ){
			if( !haveoffset ){
				memmove(raw+aligned, raw, size-aligned);
			}
			hm = (void*)((uintptr_t)raw+aligned);
			hm->flags |= M_FLAG_OFF16;
		}
		else{
			hm = (void*)((uintptr_t)raw+aligned);
			hm->flags |= haveoffset;
		}
		iassert(hm%32 == 0);
	}
	hm->count = count;
	void* ret = hm_toaddr(hm);
	iassert( ADDR(ret) % sizeof(uintptr_t) == 0 );
	return ret;
}

void m_free(void* addr){
	dbg_info("free addr: %p", addr);
	if( !addr ) return;
	hmem_s* hm = m_header(addr);
	if( !hm->refs ) die("double free");
	if( --hm->refs ) return;
	if( hm->cleanup ) hm->cleanup(addr);
	void* raw = hm->flags & M_FLAG_OFF16? (void*)((uintptr_t)hm - 16): hm;
	free(raw);
}

void* m_borrowed(void* mem){
	if( !mem ) return mem;
	++m_header(mem)->refs;
	return mem;
}

void* m_delete(void* mem, size_t index, size_t count){
	hmem_s* hm = m_header(mem);
	if( !count || index >= hm->len){
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

void* m_widen(void* mem, size_t index, size_t count){
	if( !count ){
		errno = EINVAL;
		return mem;
	}
	mem = m_grow(mem, count);
	hmem_s* hm = m_header(mem);
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

void* m_insert(void* restrict dst, size_t index, void* restrict src, size_t count){
	errno = 0;
	dst = m_widen(dst, index, count);
	if( errno ) return dst;	
	const unsigned sof = m_header(dst)->sof;
	void* draw = (void*)ADDRTO(dst, sof, index);
	memcpy(draw, src, count * sof);
	return dst;
}

void* m_shuffle(void* mem, size_t begin, size_t end){
	hmem_s* hm = m_header(mem);
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

