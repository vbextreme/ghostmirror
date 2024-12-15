#ifndef __GM_H__
#define __GM_H__

typedef enum{
	O_a,
	O_m,
	O_c,
	O_C,
	O_u,
	O_d,
	O_O,
	O_p,
	O_P,
	O_o,
	O_s,
	O_S,
	O_l,
	O_L,
	O_T,
	O_i,
	O_D,
	O_t,
	O_f,
	O_h
}OPT_E;

char* path_home(char* path);
char* path_explode(const char* path);



#endif
