#undef DBG_ENABLE

#include <notstd/type.h>
#include <notstd/memory.h>
#include <notstd/debug.h>

#include <string.h>
#include <errno.h>

/////////////////////////////////////
/////////////////////////////////////
/////////////////////////////////////
/////////////////////////////////////
/////////////////////////////////////
//// memswap same memcpy/memmove ////
/////////////////////////////////////
/////////////////////////////////////
/////////////////////////////////////
/////////////////////////////////////
/////////////////////////////////////

__private void swap_(char* restrict a, char* restrict b, size_t size){
	uintptr_t offsetA = (uintptr_t)a % sizeof(unsigned long);
	if( offsetA != (uintptr_t)b % sizeof(unsigned long) ) offsetA = size;
	
	dbg_info("mode addralign %lu", offsetA);
	size -= offsetA;
	while( offsetA-->0 ){
		char tmp = *a;
		*a = *b;
		*b = tmp;
		++a;
		++b;
	}
	
	size_t fastsize = size / sizeof(unsigned long);
	size -= (fastsize*sizeof(unsigned long));
	dbg_info("mode ulong %lu", fastsize);
	unsigned long* A = __assumealigned(a, sizeof(unsigned long));
	unsigned long* B = __assumealigned(b, sizeof(unsigned long));
	while( fastsize--> 0 ){
		unsigned long tmp = *A;
		*A = *B;
		*B = tmp;
		++A;
		++B;
	}

	a = (char*)A;
	b = (char*)B;
	dbg_info("lastes %lu", size);
	while( size--> 0 ){
		char tmp = *a;
		*a = *b;
		*b = tmp;
		++a;
		++b;
	}
}

int memswap(void* restrict a, size_t sizeA, void* restrict b, size_t sizeB){
	if( !a || !b || !sizeA || !sizeB ){
		dbg_error("unable swap %p and %p", a, b);
		errno = EINVAL;
		return -1;
	}

	if( sizeA == sizeB ){
		dbg_info("swap equal size");
		swap_(a, b, sizeA);
	}
	else if( sizeA > sizeB ){
		dbg_info("swap A > B (%lu-%lu=%lu)", sizeA, sizeB, sizeA - sizeB);
		//char testA[4096], testB[4096];
		//memcpy(testA, a, sizeA);
		//memcpy(testB, b, sizeB);
		size_t size = sizeA - sizeB;
		//dbg_info("swap %lu equal size bytes", sizeB);
		swap_(a, b, sizeB);
		//if( memcmp(testA, b, sizeB ) ) die("testA fail");	
		//if( memcmp(testB, a, sizeB ) ) die("testB fail");
		memcpy((void*)((uintptr_t)b+sizeB), (void*)((uintptr_t)a+sizeB), size);
		//if( memcmp(testA, b, sizeA ) ) die("full testA fail");
		//if( memcmp(testB, a, sizeB ) ) die("full testB fail");
	}
	else{
		dbg_info("swap A < B");
		size_t size = sizeB - sizeA;
		swap_(a, b, sizeA);
		memcpy((void*)((uintptr_t)a+sizeA), (void*)((uintptr_t)b+sizeA), size);
	}

	return 0;
}


