#ifndef __WWW_H__
#define __WWW_H__

void www_begin(void);
void www_end(void);
const char* www_errno_str(void);
void* www_download(const char* url, unsigned onlyheader, unsigned touts, char** realurl);
void* www_download_retry(const char* url, unsigned onlyheader, unsigned touts, unsigned retry, unsigned retryms, char** realurl);

#endif
