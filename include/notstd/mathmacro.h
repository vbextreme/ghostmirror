#ifndef __NOTSTD_MATH_MACRO_H__
#define __NOTSTD_MATH_MACRO_H__

#define KiB (1024UL)
#define MiB (KiB*KiB)
#define GiB (MiB*MiB)
#define PI	M_PI
#define PI2	(2.0*PI)
#define	RAD	(PI/180.0)
#define SMALL_FLOAT	(1e-12)
#define ANGLE_FULL 360
#define DOUBLE_CMP(A,B,R) ((A) > (B) - (R) && (A) < (B) + (R))
#define MAX(A,B) ((A>B)?A:B)
#define MAX3(A,B,C) MAX(MAX(A,B),C)
#define MIN(A,B) ((A<B)?A:B)
#define MIN3(A,B,C) MIN(MIN(A,B),C)

//((((N)+(S)-1)/(S))*(S))
#define ROUND_UP(N,S) ({\
	typeof(N) n = N;\
	typeof(S) s = S;\
	((n+s-1)/s)*s;\
})

#define ROUND_DOWN(N,S) ({\
	typeof(N) n = N;\
	typeof(S) s = S;\
	(n/s)*s;\
})

#define ROUND_UP_POW_TWO32(N) ({\
	   	unsigned int r = (N);\
	   	--r;\
		r |= r >> 1;\
		r |= r >> 2;\
		r |= r >> 4;\
		r |= r >> 8;\
		r |= r >> 16;\
		++r;\
		r;\
	})

#define ROUND_DOWN_POW_TWO32(N) ({\
	   	unsigned int r = ROUND_UP_POW_TWO32((N)+1);\
		r >> 1;\
	})

#define FAST_MOD_POW_TWO(N,M) ((N) & ((M) - 1))

/*- check if numbers is modulo of power of two @>0 'numbers' @< '1 if is pow of two, otherwise 0'*/
#define IS_POW_TWO(N) (!((N) & ((N) - 1)))

#define FAST_BIT_COUNT(B) __builtin_popcount(B)

#define FAST_COUNT_0_BIT_LEFT(B) __builtin_clz(B)

#define FAST_COUNT_0_BIT_RIGHT(B) __builtin_ctz(B)

#define BIG_TO_LITTLE32(NUM) (((NUM>>24)&0xFF) | ((NUM<<8)&0xFF0000) | ((NUM>>8)&0xFF00) | ((NUM<<24)&0xFF000000))

#define MM_ALPHA(CHANNEL,COUNT) ((CHANNEL)/((COUNT)+1))
#define MM_AHPLA(CHANNEL,ALPHA) ((CHANNEL/2)-ALPHA)
#define MM_NEXT(CHANNEL,ALPHA,AHPLA,NEWVAL,OLDVAL) (((ALPHA)*(NEWVAL)+(AHPLA)*(OLDVAL))/(CHANNEL/2))

#define US_MS(MS) ((MS)*1000U)
#define US_S(S)   ((S)*1000000U)

/*- get max value of variable
 * @< '_MAX of variable type'
 * @>0 'variable'
 */
#define VAR_MAX(V) ((1<<sizeof(V)*8)-1)

#endif
