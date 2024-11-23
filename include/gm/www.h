#ifndef __WWW_H__
#define __WWW_H__

void www_begin(void);
void www_end(void);
const char* www_errno_str(void);
void* www_mdownload(const char* url, unsigned touts);
void* www_mdownload_retry(const char* url, unsigned touts, unsigned retry, unsigned retryms);
char* www_header_get(const char* url, unsigned touts);

#endif
