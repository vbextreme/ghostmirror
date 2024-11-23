//#undef DBG_ENABLE

#include <notstd/delay.h>

#ifdef OS_LINUX
#include <utime.h>
#include <time.h>
#include <sys/time.h>
#include <sys/sysinfo.h>

delay_t time_ms(void){
	struct timespec ts; 
	clock_gettime(CLOCK_REALTIME, &ts); 
    return ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL;
}

delay_t time_us(void){
	struct timespec ts; 
	clock_gettime(CLOCK_REALTIME, &ts); 
    return ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000ULL;
}

delay_t time_ns(void){
	struct timespec ts; 
	clock_gettime(CLOCK_REALTIME, &ts); 
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

delay_t time_cpu_ms(void){
	struct timespec ts; 
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts); 
    return ts.tv_sec * 1000UL + ts.tv_nsec / 1000000UL;
}

delay_t time_cpu_us(void){
	struct timespec ts; 
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts); 
    return ts.tv_sec * 1000000UL + ts.tv_nsec / 1000UL;
}

delay_t time_cpu_ns(void){
	struct timespec ts; 
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts); 
    return ts.tv_sec * 1000000000UL + ts.tv_nsec;
}

double time_sec(void){
	struct timespec ts;  
	clock_gettime(CLOCK_REALTIME, &ts); 
    return ts.tv_sec + ts.tv_nsec*1e-9;
}

__private void timespec_ms(struct timespec* tv, uint64_t ms){
	tv->tv_sec = (time_t) ms / 1000;
	tv->tv_nsec = (long) ((ms - (tv->tv_sec * 1000)) * 1000000L);
}

__private void timespec_us(struct timespec* tv, uint64_t us){
	tv->tv_sec = (time_t) us / 1000000L;
	tv->tv_nsec = (long) ((us - (tv->tv_sec * 1000000L)) * 1000L);
}

__private void timespec_ns(struct timespec* tv, uint64_t us){
	tv->tv_sec = (time_t) us / 1000000000L;
	tv->tv_nsec = (long)(us - (tv->tv_sec * 1000000000L));
}

__private void timespec_sec(struct timespec* tv, double s){
	tv->tv_sec = (time_t) s;
	tv->tv_nsec = (long) ((s - tv->tv_sec) * 1e+9);
}

__private void timespec_wait(struct timespec* tv, size_t(*gettime_f)(void), size_t time){
	size_t start = gettime_f();
	while (1){
		int rval = nanosleep(tv, tv);
		if( !rval ) return;
		else if( errno == EINTR ) continue;
		else{
			while( gettime_f() - start < time );
			return;
		}
	}
}

void delay_ms(delay_t ms){
	struct timespec tv;
	timespec_ms(&tv, ms);
	timespec_wait(&tv, time_ms, ms);
}

void delay_us(delay_t us){
	struct timespec tv;
	timespec_us(&tv, us);
	timespec_wait(&tv, time_us, us);
}

void delay_ns(delay_t ns){
	struct timespec tv;
	timespec_ns(&tv, ns);
	timespec_wait(&tv, time_ns, ns);
}

void delay_sec(double s){
	struct timespec tv;
	timespec_sec(&tv, s);
	timespec_wait(&tv, time_us, s * 1000000.0);
}

void delay_hard(delay_t us){
	delay_t start = time_us();
	while( time_us() - start < us );
}

#else
__error("todo need implementation");
#endif
