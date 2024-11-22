#ifndef __NOTSTD_TYPE_H__
#define __NOTSTD_TYPE_H__

#include <stdint.h>
#include <unistd.h>

typedef uintptr_t align_t;

#if defined __x86_64__
	typedef volatile uint64_t* reg_t;
#else
	__error("unknow arch");	
#endif

#if defined __clang__
//#warning C2X define _Decimal* but clang not provide
	typedef float __Decimal32;
	typedef double __Decimal64;
	#define DECIMAL32_FORMAT "f"
	#define DECIMAL64_FORMAT "f"
#else
	typedef _Decimal32 __Decimal32;
	typedef _Decimal64 __Decimal64;
	//todo gcc not supprt format
	#define DECIMAL32_FORMAT "H"
	#define DECIMAL64_FORMAT "D"
#endif

#if defined __linux__
	#define OS_LINUX
#else
	#define OS_BAREMETAL
#endif

typedef int (*cmp_f)(const void* a, const void* b);

typedef int err_t;
typedef enum { FALSE, TRUE } bool_t;
typedef __Decimal64 decimal_t;
typedef __Decimal32 short_decimal_t;


#endif
