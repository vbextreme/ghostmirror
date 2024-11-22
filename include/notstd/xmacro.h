#ifndef __NOTSTD_XMACRO_H__
#define __NOTSTD_XMACRO_H__

#define __VA_COUNT_IMPL__(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,N,...) N
#define VA_COUNT(arg...) __VA_COUNT_IMPL__(_SkIp_, ##arg,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)
#define EXPAND(M) M
#define CONCAT(A,B) A##B
#define CONCAT_EXPAND(A,B) CONCAT(A,B)
#define CONSTRUCT(ST, EN) for(int __tmp__ = 1; __tmp__ && ST; __tmp__ = 0, EN )
#define EXPAND_STRING(ES) #ES

#define _Y_ 1
#define _N_ 0


#endif
