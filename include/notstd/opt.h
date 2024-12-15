#ifndef __OPT_H__
#define __OPT_H__

#include <stdint.h>

#define OPT_NOARG  0x0000
#define OPT_STR    0x0001
#define OPT_NUM    0x0002
#define OPT_INUM   0x0003
#define OPT_FNUM   0x0004
#define OPT_PATH   0x0005
#define OPT_REPEAT 0x0100
#define OPT_EXTRA  0x0200
#define OPT_ARRAY  0x0400
#define OPT_END    0x0800
#define OPT_EXISTS 0x1000
#define OPT_DIR    0x2000
#define OPT_TYPE   0x00FF

typedef struct option option_s;

typedef union optValue{
	const char*   str;
	unsigned long ui;
	long          i;
	double        f;
}optValue_u;

typedef struct option{
	const char  sh;
	const char* lo;
	const char* desc;
	unsigned    flags;
	unsigned    set;
	optValue_u* value;
}option_s;

#define __argv __cleanup(argv_cleanup)

option_s* argv_parse(option_s* opt, int argc, char** argv);
option_s* argv_dtor(option_s* opt);
void argv_cleanup(void* ppopt);
void argv_usage(option_s* opt, const char* argv0);
void argv_default_str(option_s* opt, unsigned id, const char* str);
void argv_default_num(option_s* opt, unsigned id, unsigned long ui);
void argv_default_inum(option_s* opt, unsigned id, long i);
void argv_default_fnum(option_s* opt, unsigned id, double f);


#endif
