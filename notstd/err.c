#include <notstd/type.h>
#include <notstd/compiler.h>
#include <notstd/error.h>

#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

__private unsigned termwidth(void){
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	return w.ws_col;
}

__private const char* err_startline(const char* begin, const char* str){
	while( str > begin && *str != '\n' ) --str;
	return *str == '\n' ? ++str : str;
}

__private unsigned err_countline(const char* begin, const char* line){
	unsigned count = 1;
	while( *(begin = strchrnul(begin, '\n')) && begin < line ){
		++begin;
		++count;
	}
	return count;
}

void err_showline(const char* begin, const char* err, unsigned nch){
	int __tty__ = isatty(fileno(stderr));
	const char* __ce__ = __tty__ ? ERR_COLOR : "";\
	const char* __cr__ = __tty__ ? ERR_COLOR_RESET : "";\
	const char* __cm__ = __tty__ ? ERR_COLOR_EV : "";\
	const char* line = err_startline(begin, err);
	unsigned nline  = *strchrnul(begin, '\n') ? err_countline(begin, line) : 0;
	unsigned width  = termwidth();
	unsigned poserr = err - line;

	if( nline > 0 )	fprintf(stderr, "at line: %u\n", nline);
	
	unsigned x = 0;
	unsigned y = 0;
	while( *line && *line != '\n' ){
		if( y && x >= poserr && x <= poserr+nch ){
			fputs(__cm__, stderr);
			fputc(*line++, stderr);
			fputs(__cr__, stderr);
		}
		else{
			fputc(*line++, stderr);
		}
		++x;
		if( x >= width ){
			x = 0;
			++y;
		}
	}
	fputc('\n', stderr);

	unsigned sp = poserr % width;
	for( unsigned i = 0; i < sp; ++i ) fputc(' ', stderr);
	fputs(__ce__, stderr);
	fputc('^', stderr);
	for( unsigned i = 2; i < nch; ++i ) fputc('~', stderr);
	if( nch > 1 ) fputc('^', stderr);
	fputs(__cr__, stderr);
}

