#ifndef __NOTSTD_ATOMIC_H__
#define __NOTSTD_ATOMIC_H__

#include <stdatomic.h>

#define __atomic volatile _Atomic

//A is __atomic pointer
//T is __atomic pointer
//V is value
// *a = v
#define a_store_relaxed(A, V) atomic_store_explicit(A, V, memory_order_relaxed)
#define a_store_release(A, V) atomic_store_explicit(A, V, memory_order_release)

// return *a
#define a_load_relaxed(A) atomic_load_explicit(A, memory_order_relaxed)
#define a_load_acquire(A) atomic_load_explicit(A, memory_order_acquire)

// tmp = *a; *a = v; return tmp
#define a_exchange_relaxed(A, V) atomic_exchange_explicit(A, V, memory_order_relaxed)
#define a_exchange_acquire(A, V) atomic_exchange_explicit(A, V, memory_order_acquire)
#define a_exchange_release(A, V) atomic_exchange_explicit(A, V, memory_order_release)

//compare and swap
// *a == *t ? *a = v && ret = 1 : ret = 0; *t = *a;
#define a_cas_strong_relaxed_relaxed(A, T, V) atomic_compare_exchange_strong_explicit(A, T, V, memory_order_relaxed, memory_order_relaxed)
#define a_cas_strong_acquire_relaxed(A, T, V) atomic_compare_exchange_strong_explicit(A, T, V, memory_order_acquire, memory_order_relaxed)
#define a_cas_strong_release_relaxed(A, T, V) atomic_compare_exchange_strong_explicit(A, T, V, memory_order_release, memory_order_relaxed)
#define a_cas_weak_relaxed_relaxed(A, T, V) atomic_compare_exchange_weak_explicit(A, T, V, memory_order_relaxed, memory_order_relaxed)
#define a_cas_weak_acquire_relaxed(A, T, V) atomic_compare_exchange_weak_explicit(A, T, V, memory_order_acquire, memory_order_relaxed)
#define a_cas_weak_release_relaxed(A, T, V) atomic_compare_exchange_weak_explicit(A, T, V, memory_order_release, memory_order_relaxed)


// fetch and operations
// tmp = *a; *a += v; return tmp
#define a_fadd_relaxed(A, V) atomic_fetch_add_explicit(A, V, memory_order_relaxed)
#define a_fadd_acquire(A, V) atomic_fetch_add_explicit(A, V, memory_order_acquire)
#define a_fadd_release(A, V) atomic_fetch_add_explicit(A, V, memory_order_release)

// tmp = *a; *a -= v; return tmp
#define a_fsub_relaxed(A, V) atomic_fetch_sub_explicit(A, V, memory_order_relaxed)
#define a_fsub_acquire(A, V) atomic_fetch_sub_explicit(A, V, memory_order_acquire)
#define a_fsub_release(A, V) atomic_fetch_sub_explicit(A, V, memory_order_release)


#endif
