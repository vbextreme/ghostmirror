#ifndef __NOTSTD_ERROR_H__
#define __NOTSTD_ERROR_H__

#if defined __linux__
	#include <errno.h>
	#include <stdlib.h>
#else
	__error("todo");
#endif

#define ERR_COLOR       "\033[31m"
#define ERR_COLOR_INFO  "\033[36m"
#define ERR_COLOR_EV    "\033[46m"
#define ERR_COLOR_RESET "\033[m"

#define err(FORMAT, arg...) do{\
	int __tty__ = isatty(fileno(stderr));\
	const char* __ce__ = __tty__ ? ERR_COLOR : "";\
	const char* __ci__ = __tty__ ? ERR_COLOR_INFO : "";\
	fprintf(stderr, "%serror:%s " FORMAT "\n", __ce__, __ci__, ## arg); \
	if( __tty__ ) fputs(ERR_COLOR_RESET, stderr);\
}while(0)

#define ern(FORMAT, arg...) do{\
	int __tty__ = isatty(fileno(stderr));\
	const char* __ce__ = __tty__ ? ERR_COLOR : "";\
	const char* __ci__ = __tty__ ? ERR_COLOR_INFO : "";\
	fprintf(stderr, "%serror:%s " FORMAT , __ce__, __ci__, ## arg);\
	if( __tty__ ) fputs(ERR_COLOR_RESET, stderr);\
	fprintf(stderr, " (%d)::%s\n", errno, err_str() );\
}while(0)

/*- stop software and exit arguments is same printf */
#if DBG_ENABLE > 0
#define die(FORMAT, arg...) do{\
	int __tty__ = isatty(fileno(stderr));\
	const char* __ce__ = __tty__ ? ERR_COLOR : "";\
	const char* __ci__ = __tty__ ? ERR_COLOR_INFO : "";\
	fprintf(stderr, "%sdie%s at %s(%u) " FORMAT "\n", __ce__, __ci__, __FILE__, __LINE__, ## arg);\
	if( __tty__ ) fputs(ERR_COLOR_RESET, stderr);\
	fflush(stderr);\
	fsync(3);\
	exit(1);\
}while(0)
#else
#define die(FORMAT, arg...) do{\
	int __tty__ = isatty(fileno(stderr));\
	const char* __ce__ = __tty__ ? ERR_COLOR : "";\
	const char* __ci__ = __tty__ ? ERR_COLOR_INFO : "";\
	fprintf(stderr, "%sdie%s " FORMAT "\n", __ce__, __ci__, ## arg);\
	if( __tty__ ) fputs(ERR_COLOR_RESET, stderr);\
	exit(1);\
}while(0)
#endif

void err_showline(const char* begin, const char* err, unsigned nch);

#endif
