#ifndef __INUTILITY_H__
#define __INUTILITY_H__

#include <notstd/core.h>

#define FORMAT_RESET        "\033[0m"
#define FORMAT_FG           "\033[38;5;%um"
#define FORMAT_BG           "\033[48;5;%um"
#define FORMAT_BOLD         "\033[1m"
#define FORMAT_CLS_LINE     "\033[2K"
#define FORMAT_CURSOR_STORE "\033[s"
#define FORMAT_CURSOR_LOAD  "\033[u"
#define FORMAT_GOTO_YX      "\033[%u;%uH"
#define FORMAT_LAST_Y       "\033[999;1H"
#define FORMAT_HOME         "\033[u"
#define FORMAT_UPN	        "\033[%uA"
#define FORMAT_UP	        "\033[1A"

#define COLOR_ERR 9
#define COLOR_OK  46
#define COLOR_BAR_FG 44
#define COLOR_BAR_BG 240

char* path_home(char* path);
char* path_explode(const char* path);
unsigned cpu_cores(void);
char* load_file(const char* fname, int txt, int exists);
void term_wh(unsigned* w, unsigned* h);
void term_scroll_region(unsigned y1, unsigned y2);
__dtor_priority(1000) void term_status_line_end(void);
void term_status_line_begin(void);
void progress_enable(int enable);
void progress_begin(const char* desc);
void progress_end(void);
void progress_refresh(const char* msg, unsigned value, unsigned count);
void progress_status_refresh(const char* status, unsigned color, const char* msg, unsigned value, unsigned count);
void term_fg(unsigned color);
void term_bg(unsigned color);
void term_bold(void);
void print_repeats(unsigned count, const char* ch);
void print_repeat(unsigned count, const char ch);

#endif
