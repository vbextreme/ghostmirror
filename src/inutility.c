#include <notstd/core.h>
#include <notstd/str.h>
#include <gm/inutility.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>
#include <pwd.h>
#include <limits.h>

//TODO bug if resize term and write at last line is impossible to restore status line, can resize term in sig?

char* path_home(char* path){
	char *hd;
	if( (hd = getenv("HOME")) == NULL ){
		struct passwd* spwd = getpwuid(getuid());
		if( !spwd ) die("impossible to get home directory");
		strcpy(path, spwd->pw_dir);
	}
	else{
		strcpy(path, hd);
    }
	return path;
}

char* path_explode(const char* path){
	if( path[0] == '~' && path[1] == '/' ){
		char home[PATH_MAX];
		return str_printf("%s%s", path_home(home), &path[1]);
	}
	else if( path[0] == '.' && path[1] == '/' ){
		char cwd[PATH_MAX];
		return str_printf("%s%s", getcwd(cwd, PATH_MAX), &path[1]);
	}
	else if( path[0] == '.' && path[1] == '.' && path[2] == '/' ){
		char cwd[PATH_MAX];
		getcwd(cwd, PATH_MAX);
		const char* bk = strrchr(cwd, '/');
		iassert( bk );
		return str_printf("%.*s%s", (int)(bk-cwd), cwd, &path[2]);
	}
	else if( path[0] == '/' ){
		return str_dup(path, 0);
	}
	else{
		char cwd[PATH_MAX];
		return str_printf("%s%s", getcwd(cwd, PATH_MAX), &path[1]);
	}
}


unsigned cpu_cores(void){
	return sysconf(_SC_NPROCESSORS_ONLN);
}

__private int statusEnable;
__private unsigned TERM_H;

void term_wh(unsigned* w, unsigned* h){
	struct winsize ws;
    ioctl(0, TIOCGWINSZ, &ws);
	if( w ) *w = ws.ws_col;
	if( h ) *h = ws.ws_row;
}

void term_scroll_region(unsigned y1, unsigned y2){
	dprintf(STDOUT_FILENO,
		"\n"
		FORMAT_UP
		FORMAT_CURSOR_STORE
		"\033[%u;%ur"
		FORMAT_CURSOR_LOAD
		,y1, y2
	);
}

__dtor_priority(1000)
void term_status_line_end(void){
	if( statusEnable ){
		term_wh(NULL, &TERM_H);
		
		dprintf(STDOUT_FILENO,
			FORMAT_CURSOR_STORE
			FORMAT_GOTO_YX
			FORMAT_CLS_LINE
			FORMAT_CURSOR_LOAD
			,TERM_H, 1
		);
		
		term_scroll_region(1, TERM_H);
		statusEnable = 0;
	}
}

void sigint_handler(int sig) {
	if( sig == SIGINT ){
		term_status_line_end();
		exit(0);
	}
}

void sigwinch_handler(int sig) {
	if( sig == SIGWINCH ){
		term_wh(NULL, &TERM_H);
	}
}

void term_status_line_begin(void){
	if( !statusEnable ){
		term_wh(NULL, &TERM_H);
		term_scroll_region(1, TERM_H-1);
		statusEnable = 1;
		signal(SIGINT, sigint_handler);
		signal(SIGWINCH, sigwinch_handler);
	}
}

__private const char* PROGRESS_DESC;
__private int PROGRESS_ENABLE;

void progress_enable(int enable){
	PROGRESS_ENABLE = enable;
}

void progress_begin(const char* desc){
	if( !PROGRESS_ENABLE ) return;
	PROGRESS_DESC = desc;
	dprintf(STDOUT_FILENO,
		FORMAT_CURSOR_STORE
		FORMAT_GOTO_YX
		FORMAT_CLS_LINE
		"%s"
		FORMAT_CURSOR_LOAD
		,TERM_H, 1, desc
	);
}

void progress_end(void){
	if( !PROGRESS_ENABLE ) return;
	dprintf(STDOUT_FILENO,
		FORMAT_CURSOR_STORE
		FORMAT_GOTO_YX
		FORMAT_CLS_LINE
		FORMAT_CURSOR_LOAD
		,TERM_H, 1
	);
}

void progress_refresh(const char* msg, unsigned value, unsigned count){
	if( !PROGRESS_ENABLE ) return;
	dprintf(STDOUT_FILENO,
		FORMAT_CURSOR_STORE
		FORMAT_GOTO_YX
		FORMAT_CLS_LINE
		"[%5.1f%%] %s"
		FORMAT_CURSOR_LOAD
		"%s\n"
		,TERM_H, 1, (double)value*100.0/(double)count, PROGRESS_DESC, msg
	);
}

void progress_status_refresh(const char* status, unsigned color, const char* msg, unsigned value, unsigned count){
	switch( PROGRESS_ENABLE ){
		case 1:
			dprintf(STDOUT_FILENO,
				FORMAT_CURSOR_STORE
				FORMAT_GOTO_YX
				FORMAT_CLS_LINE
				"[ %5.1f%% ] %s"
				FORMAT_CURSOR_LOAD
				"[ %s ] %s\n"
				,TERM_H, 1, (double)value*100.0/(double)count, PROGRESS_DESC, status, msg
			);
		break;

		case 2:
			dprintf(STDOUT_FILENO,
				FORMAT_CURSOR_STORE
				FORMAT_GOTO_YX
				FORMAT_CLS_LINE
				"[ %5.1f%% ] %s"
				FORMAT_CURSOR_LOAD
				"[ " FORMAT_FG "%s" FORMAT_RESET " ] %s\n"
				,TERM_H, 1, (double)value*100.0/(double)count, PROGRESS_DESC, color, status, msg
			);
		break;
	}
}


