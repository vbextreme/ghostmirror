#ifndef __WWW_H__
#define __WWW_H__

void www_begin(unsigned maxthr);
void www_end(void);
const char* www_errno_str(void);
unsigned www_errno(void);
unsigned www_connection_error(unsigned error);
unsigned www_http_error(unsigned error);
const char* www_str_error(unsigned error);
unsigned www_gzstate(void);

void* www_download(const char* url, unsigned onlyheader, unsigned touts, char** realurl);
void* www_download_gz(const char* url, unsigned onlyheader, unsigned touts, char** realurl);
char* www_host_get(const char* url);
long www_ping(const char* host);

#endif
