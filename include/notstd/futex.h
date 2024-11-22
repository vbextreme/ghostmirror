#ifndef __NOTSTD_LINUX_FUTEX_H__
#define __NOTSTD_LINUX_FUTEX_H__

#ifndef __linux__
__error("todo need porting futex");
#endif

#include <time.h>
#include <linux/futex.h>

#ifndef FUTEX_32
#define FUTEX_32 2
#endif

#ifndef FUTEX_WAITV_MAX
#define FUTEX_WAITV_MAX		128
#endif

int futex_to(int *uaddr, int futex_op, int val, const struct timespec *timeout, int *uaddr2, int val3);
int futex_v2(int *uaddr, int futex_op, int val, unsigned val2, int *uaddr2, int val3);

/** overloading for futex*/
#define futex(ADDR, OP, VAL, V2TO, ADDR2, VAL3) _Generic((V2TO),\
	const struct timespec*: futex_to,\
	void*: futex_to,\
	unsigned: futex_v2\
)(ADDR, OP, VAL, V2TO, ADDR2, VAL3)	

typedef struct futex_waitv futexWaitv_s;

int futex_waitv(struct futex_waitv *waiters, unsigned int nr_futexes, unsigned int flags);
int futex_waitv_to(futexWaitv_s *waiters, unsigned int nr_futexes, unsigned int flags, struct timespec *timeout, clockid_t clockid);
int futex_waitv_ms(futexWaitv_s *waiters, unsigned int nr_futexes, unsigned int flags, long ms);
int futex_waitv_us(futexWaitv_s *waiters, unsigned int nr_futexes, unsigned int flags, long us);

#endif
