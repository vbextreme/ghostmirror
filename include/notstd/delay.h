#ifndef __NOTSTD_CORE_DELAY_H__
#define __NOTSTD_CORE_DELAY_H__

#include <notstd/core.h>

#define SECTONS(S) ((S)*1000000000UL)
#define SECTOUS(S) ((S)*1000000UL)
#define SECTOMS(S) ((S)*1000UL)

#define MSTONS(MS) ((MS)*1000000UL)
#define MSTOUS(MS) ((MS)*1000UL)
#define MSTOSE(MS) ((MS)/1000UL)

#define USTONS(US) ((US)*1000UL)
#define USTOMS(US) ((US)/1000UL)
#define USTOSE(US) ((US)/1000000UL)

#define NSTOUS(NS) ((NS)/1000UL)
#define NSTOMS(NS) ((NS)/1000000UL)
#define NSTOSE(NS) ((NS)/1000000000UL)

typedef uint64_t delay_t;

delay_t time_ms(void);
delay_t time_us(void);
delay_t time_ns(void);

delay_t time_cpu_ms(void);
delay_t time_cpu_us(void);
delay_t time_cpu_ns(void);

double time_sec(void);

void delay_ms(delay_t ms);
void delay_us(delay_t us);
void delay_ns(delay_t ns);
void delay_sec(double s);

void delay_hard(delay_t us);

#endif
