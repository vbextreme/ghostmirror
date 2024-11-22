#ifndef __NOTSTD_CORE_THREADS_H__
#define __NOTSTD_CORE_THREADS_H__

#include <notstd/core.h>
#include <notstd/list.h>

/********************/
/*** generic lock ***/
/********************/

#define GLOCK_MAX_WAITABLE 128

typedef struct glock glock_s;

typedef int(*glockWait_f)(glock_s* gl, int* waitval);

struct glock{
	int futex;
	int private;
	int state;
	void* ctx;
	glockWait_f wait;
};

glock_s* glock_ctor(glock_s* gl, int val, int sharedProcess, glockWait_f w);
void glock_wait(glock_s* gl, int value);
void glock_wake(glock_s* gl);
void glock_broadcast(glock_s* gl);

//if want use anyof or waitv you can lock abstraction of object glock calling methods *_await_*
//so, examples, can call mutex_lock but after lock can't use anyof/waitv, you can use only after unlock
//but in complex examples where you wait many object if one obje are unlocked ad want wait any in other list you need recall anyof/waitv or otherwise
//	mutex_await_lock(...) or *_await_*(...) function
glock_s* glock_anyof(glock_s** gls, unsigned count);
void glock_waitv(glock_s** gls, unsigned count);

//is same to call mutex_lock or sem_wait or event_wait is better for use with glock_anyof, waitv
void glock_await(glock_s* gl);

/*************/
/*** mutex ***/
/*************/

/* init a mutex if you have create without mutex_new*/
glock_s* mutex_ctor(glock_s* mtx, int sharedProcess);

/* unlock mutex */
int mutex_unlock(glock_s* mtx);

/* lock mutex */
int mutex_lock(glock_s* mtx);

/* try to lock mutex 
 * @param mtx mutex
 * @return 0 if lock mutex, -1 if other thread have locked the mutex
 */
int mutex_trylock(glock_s* mtx);

#define mutex_guard(MTX) for( int _guard_ = mutex_lock(MTX); _guard_; _guard_ = 0, mutex_unlock(MTX) )

/*****************/
/*** semaphore ***/
/*****************/

glock_s* semaphore_ctor(glock_s* sem, int val, int sharedProcess);

/* increment by 1 the sempahore, if sem is 0 wake 1 thread */
void semaphore_post(glock_s* sem);

/* decrement by 1 the semaphore, if sem is 0 call function started wait thread*/
void semaphore_wait(glock_s* sem);

/* try to decrement semaphore, if semaphore is 0 return -1 and not wait
 * @param sem semaphore
 * @return 0 if sem is decremented -1 otherwise
 */
int semaphore_trywait(glock_s* sem);

/*************/
/*** event ***/
/*************/

glock_s* event_ctor(glock_s* ev, int sharedProcess);

/* wake all thread that waited on this event */
void event_raise(glock_s* ev);

void event_broadcast(glock_s* ev);

/* wait a event raised */
void event_wait(glock_s* ev);

void event_clear(glock_s* ev);

int event_israised(glock_s* ev);

/***************/
/*** eventfd ***/
/***************/

typedef void (*evfd_onread_f)(int err, int fd, uint64_t value, void* ctx);

/* create eventfd
 * @param val begin value
 * @param nonblock set nonblock event
 * @return fd or -1 error
 */
int event_fd(long val, int nonblock);

/* read value from fd, when the eventfd counter has a nonzero value, then a read(2) returns 8 bytes containing that value, and the counter's value is reset to zero.If the eventfd counter is zero at the time of the call to read(2), then the call either blocks until the counter becomes nonzero (at which time, the read(2) proceeds as described above) or fails with the error EAGAIN if the file descriptor has been made nonblocking.
 * @param val value to read, val is setted to writeval + prevval every time is called from write before read
 * @param fd where read
 * @return 0 ok -1 error
 */
long event_fd_read(int fd);

/* write value to fd, call adds the 8-byte integer value supplied in its buffer to the counter, The maximum value that may be stored in the counter is the largest unsigned 64-bit value minus 1, If the addition would cause the counter's value to exceed the maximum, then the write(2) either blocks until a read(2) is performed on the file descriptor, or fails with the error EAGAIN if the file descriptor has been made nonblocking, will fail with the error EINVAL if the size of the supplied buffer is less than 8 bytes, or if an attempt is made to write the value 0xffffffffffffffff.
 * @param fd where read
 * @param val value to read
 * @return 0 ok -1 error
 */
void event_fd_write(int fd, uint64_t val);

/**************/
/*** thread ***/
/**************/

typedef enum { THR_STATE_STOP, THR_STATE_RUN } thr_e;

typedef struct thr thr_t;

/** thread function */
typedef void(*thr_f)(thr_t* self, void* ctx);

/* create and run new thread
 * @param fn function where start new thread
 * @param arg argument passed to fn
 * @param stackSize the stacksize, 0 use default value
 * @param oncpu assign thread to cpu, 0 auto
 * @param detach 1 set not joinable thread, 0 for joinable
 * @return thread or NULL for error
 */
thr_t* thr_new(thr_f fn, void* arg, unsigned stackSize, unsigned oncpu, int detach);

#define START(FN, ARG) thr_new(FN, ARG, 0, 0, 0)

#define treturn(SELF, VAL) do{ thr_retval(SELF, VAL); return; }while(0)

/* change thread cpu
 * @param thr thread
 * @param cpu cpu 1 to N
 * @return 0 successfull -1 error
 */
void thr_cpu_set(thr_t* thr, unsigned cpu);

/* wait, aka join, a thread and return value in out
 * @param thr a thread
 * @param out return value of thread
 * @return 0 successfull -1 error
 */
void thr_wait(thr_t* thr);

/* same wait but not wait, aka try join
 * @param thr a thread
 * @param out return value of thread
 * @return 0 thread end, 1 thread run, -1 error
 */
thr_e thr_check(thr_t* thr);

/* wait, aka join, a vector of threads
 * @param vthr a vector of threads
 * @return 0 successfull -1 error
 */
void thr_waitv(thr_t** thr, unsigned count);

/* wait any of vector thr 
 * @param vthr a vector of thr
 * @param out return value of thread
 * @return thread exited
 */
thr_t* thr_anyof(thr_t** thr, unsigned count);

/* cancel a thread
 * @param thr thread to cancel
 * @return 0 successfull, -1 error
 */
void thr_stop(thr_t* thr);

void thr_yield(void);

unsigned long thr_id(thr_t* thr);

void thr_retval(thr_t* thr, void* val);

#endif
