#ifndef __WWW_H__
#define __WWW_H__

void www_begin(void);
void www_end(void);
const char* www_errno_str(void);
unsigned www_errno(void);
unsigned www_connection_error(unsigned error);
unsigned www_http_error(unsigned error);
const char* www_str_error(unsigned error);

void* www_download(const char* url, unsigned onlyheader, unsigned touts, char** realurl);
void* www_download_retry(const char* url, unsigned onlyheader, unsigned touts, unsigned retry, unsigned retryms, char** realurl, unsigned* nretry);
long www_ping_sh(const char* url);
char* www_host_get(const char* url);
long www_ping(const char* host);

#endif
