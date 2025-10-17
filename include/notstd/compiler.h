#ifndef __NOTSTD_COMPILER_H__
#define __NOTSTD_COMPILER_H__

#define __error(STR) DO_PRAGMA(GCC error STR)
#define __warning(STR) DO_PRAGMA(GCC warning STR)

#ifdef __unused
#undef __unused
#endif
#ifdef __packed
#undef __packed
#endif
#ifdef __const
#undef __const
#endif
#ifdef __weak
#undef __weak
#endif

#define __out
#define __private static
#define __fallthrough       __attribute__((fallthrough))
#define __unused            __attribute__((unused))
#define __cleanup(FNC)      __attribute__((__cleanup__(FNC)))
#define __printf(FRMT,VA)   __attribute__((format(printf, FRMT, VA)))
#define __scanf(FRMT,VA)   __attribute__((format(scanf, FRMT, VA)))
#define __const             __attribute__((const))
#define __packed            __attribute__((packed))
#define __weak              __attribute__((weak))
#define __noinline          __attribute__((noinline))
#define __sectiona(SEC, CS) __attribute__((section(SEC "." EXPAND_STRING(CS))))
#define __aligned(N)        __attribute__((aligned(N)))
#define __returnaligned(N)  __attribute__((assume_aligned(N)))
#define __reta32            __returnaligned(32)
#define __noreturn          __attribute__((noreturn))
#define __returnonull       __attribute__((returns_nonnull))
#define __nonull(...)       __attribute__((nonnull(__VA_ARGS__)))
#define __deprecated(MSG)   __attribute__((deprecated(MSG)))
#define __constructor       __attribute__((constructor))
#define __destructor        __attribute__((destructor))
#define __malloc            __attribute__((malloc))
#define __mallocf(F)        __attribute__((malloc(F)))
#define __return_used       __attribute__((warn_unused_result))
#define __prv8              __deprecated("this field is private, used only in implementation")
#define __rdon              const
#define __rdwr
#define __cpu_init()        __builtin_cpu_init()
#define __resolver(NAME)    __attribute__((ifunc(#NAME)))
#define __ctor_priority(P)  __attribute__((constructor(P)))
#define __dtor_priority(P)  __attribute__((destructor(P)))
#define __compatible_type(A,B) __builtin_types_compatible_p(A,B)
#define __assumealigned(PTR, N) __builtin_assume_aligned(PTR, N)

#define _TOR_PRIORITY        101
#define _TOR_PRIORITY_CORE   64

#define __ctor         __constructor_priority(_TOR_PRIORITY + _TOR_PRIORITY_CORE)
#define __ctor_prio(N) __constructor_priority(_TOR_PRIORITY + _TOR_PRIORITY_CORE + (N))

#define __dtor         __destructor_priority(_TOR_PRIORITY + _TOR_PRIORITY_CORE)
#define __dtor_prio(N) __destructor_priority(_TOR_PRIORITY + _TOR_PRIORITY_CORE + (N))

#if defined OMP_ENABLE && !defined __clang__
	#define __parallel(A)  DO_PRAGMA(omp parallel A)
	#define __parallef     DO_PRAGMA(omp parallel for)
	#define __parallefo    DO_PRAGMA(omp parallel for ordered)
	#define __paralleft(V) DO_PRAGMA(omp parallel for num_threads(V))
	#define __parallefc(Z) DO_PRAGMA(omp parallel for collapse Z)
	#define __ordered()    DO_PRAGMA(omp ordered)
	#define parallel_num() omp_get_thread_num()
	#define parallel_max() omp_get_max_threads()
#else
	#define __parallel
	#define __parallef
	#define __paralleft(V)	__unused __typeof(V) __POOR_CLANG__ = V;
	#define __parallefc(Z)
	#define __parallefo    
	#define parallel_num() 1
	#define parallel_max() 4
	#define __ordered()    
#endif

#ifdef __cplusplus
#	define __extern_begin extern "C" {
#	define __extern_end }
#else
#	define __extern_begin
#	define __extern_end
#endif

#define DO_PRAGMA(DOP) _Pragma(#DOP)

#define __unsafe_begin       DO_PRAGMA(GCC diagnostic push)
#define __unsafe(FLAGS)      DO_PRAGMA(GCC diagnostic ignored FLAGS)
#define __unsafe_fallthrough __unsafe("-Wimplicit-fallthrough")
#define __unsafe_unused_fn   __unsafe("-Wunused-function")
#define __unsafe_deprecated  __unsafe("-Wdeprecated-declarations")
#define __unsafe_end         DO_PRAGMA(GCC diagnostic pop)

#define ADDR(VAR) ((uintptr_t)(VAR))
#define ADDRTO(VAR, SO, I) ( ADDR(VAR) + ((SO)*(I)))

#define OS_PAGE_SIZE sysconf(_SC_PAGESIZE)

#define sizeof_vector(V) (sizeof(V) / sizeof(V[0]))

#define forever() for(;;)

#define cpu_relax() __builtin_ia32_pause()

#define typeofbase(T) __typeof__(T)

#ifdef __clang__
#undef __mallocf
//#undef __nonull
#define __mallocf(F)         __attribute__((malloc))
//#define __nonull(...)        __attribute__((nonnull))
#endif

#endif
