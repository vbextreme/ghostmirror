#ifndef __WWW_H__
#define __WWW_H__

void www_begin(void);
void www_end(void);
const char* www_errno_str(void);
void* www_mdownload(const char* url, unsigned touts);
char* www_header_get(const char* url, unsigned touts);

#endif
