#include <notstd/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifndef SYS_futex_waitv
#error "your kernel is too old, upgrade it, think >= 5.16"
#endif

int futex_to(int *uaddr, int futex_op, int val, const struct timespec *timeout, int *uaddr2, int val3){
	return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
}

int futex_v2(int *uaddr, int futex_op, int val, unsigned val2, int *uaddr2, int val3){
	return syscall(SYS_futex, uaddr, futex_op, val, val2, uaddr2, val3);
}

int futex_waitv(futexWaitv_s *waiters, unsigned int nr_futexes, unsigned int flags){
	return syscall(SYS_futex_waitv, waiters, nr_futexes, flags, NULL, 0);
}

int futex_waitv_to(futexWaitv_s *waiters, unsigned int nr_futexes, unsigned int flags, struct timespec *timeout, clockid_t clockid){
	return syscall(SYS_futex_waitv, waiters, nr_futexes, flags, timeout, clockid);
}

int futex_waitv_ms(futexWaitv_s *waiters, unsigned int nr_futexes, unsigned int flags, long ms){
	if( ms < 0 ) return futex_waitv(waiters, nr_futexes, flags);
	struct timespec ts; 
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += (time_t) ms / 1000L;
	ts.tv_nsec = (long) ((ms - (ts.tv_sec * 1000L)) * 1000000L);
	return futex_waitv_to(waiters, nr_futexes, flags, &ts, CLOCK_REALTIME);
}

int futex_waitv_us(futexWaitv_s *waiters, unsigned int nr_futexes, unsigned int flags, long us){
	if( us < 0 ) return futex_waitv(waiters, nr_futexes, flags);
	struct timespec ts; 
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += (time_t) us / 1000000L;
	ts.tv_nsec = (long) ((us - (ts.tv_sec * 1000000L)) * 1000L);
	return futex_waitv_to(waiters, nr_futexes, flags, &ts, CLOCK_REALTIME);
}


