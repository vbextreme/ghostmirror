#ifndef __OPT_H__
#define __OPT_H__

#include <stdint.h>

#define OPT_NOARG  0x00
#define OPT_STR    0x01
#define OPT_NUM    0x02
#define OPT_INUM   0x03
#define OPT_FNUM   0x04
#define OPT_REPEAT 0x10
#define OPT_EXTRA  0x20
#define OPT_ARRAY  0x40
#define OPT_END    0x80

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
