#include <notstd/core.h>
#include <curl/curl.h>
#include <string.h>

#define __wcc __cleanup(www_curl_cleanup)

#define WWW_ERROR_HTTP 10000

__thread int wwwerrno;

void www_begin(void){
	curl_global_init(CURL_GLOBAL_DEFAULT);
}

void www_end(void){
	curl_global_cleanup();
}

__private const char* www_errno_http(long resCode) {
    switch (resCode) {
        case 200: return "HTTP OK";
        case 201: return "HTTP Created";
        case 202: return "HTTP Accepted";
        case 204: return "HTTP No Content";
        case 301: return "HTTP Moved Permanently";
        case 302: return "HTTP Found";
        case 400: return "HTTP Bad Request";
        case 401: return "HTTP Unauthorized";
        case 403: return "HTTP Forbidden";
        case 404: return "HTTP Not Found";
        case 500: return "HTTP Internal Server Error";
        case 502: return "HTTP Bad Gateway";
        case 503: return "HTTP Service Unavailable";
        default: return "Unknown HTTP Status Code";
    }
}

const char* www_errno_str(void){
	if( wwwerrno > WWW_ERROR_HTTP ) return www_errno_http(wwwerrno - WWW_ERROR_HTTP);
	return curl_easy_strerror(wwwerrno);
}

__private size_t www_curl_buffer_recv(void* ptr, size_t size, size_t nmemb, void* userctx){
	void* ctx = *(void**)userctx;
	const size_t sizein = size * nmemb;
	//dbg_info("in: %lu, buf.len: %u, buf.size: %lu ", sizein, mem_header(ctx)->len, mem_lenght(ctx));
	uint8_t* data = mem_upsize(ctx, sizein);
	memcpy(data + mem_header(data)->len, ptr, sizein);
	mem_header(data)->len += sizein;
	*(void**)userctx = data;
	return sizein;
}

__private void www_curl_cleanup(void* ch){
	curl_easy_cleanup(*(void**)ch);
}

__private CURL* www_curl_new(const char* url){
	CURL* ch = curl_easy_init();
	if( !ch ) die("curl init(%d): %s", errno, curl_easy_strerror(errno));
	curl_easy_setopt(ch, CURLOPT_URL, url);
	if( !strncmp(url, "https", 5) )
		curl_easy_setopt(ch, CURLOPT_SSL_VERIFYPEER, 1L);
	else
		curl_easy_setopt(ch, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(ch, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(ch, CURLOPT_VERBOSE, 0L);
	return ch;
}

__private int www_curl_perform(CURL* ch){
	CURLcode res;
	res = curl_easy_perform(ch);
	if ( res != CURLE_OK && res != CURLE_FTP_COULDNT_RETR_FILE ){
		dbg_error("perform return %d: %s", res, curl_easy_strerror(res));
		wwwerrno = res;
		return -1;
	}
	long resCode;
	curl_easy_getinfo(ch, CURLINFO_RESPONSE_CODE, &resCode);
    if( resCode != 200L && resCode != 0 ) {
		dbg_error("getinfo return %ld: %s", resCode, www_errno_http(resCode));
		wwwerrno = WWW_ERROR_HTTP + resCode;
		return -1;
    }
	return 0;
}

void* www_mdownload(const char* url, unsigned touts){
	dbg_info("'%s'", url);
	__wcc CURL* ch = www_curl_new(url);
	__free uint8_t* data = MANY(uint8_t, 1024);
	curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, www_curl_buffer_recv);
	curl_easy_setopt(ch, CURLOPT_WRITEDATA, &data);
	curl_easy_setopt(ch, CURLOPT_TIMEOUT, touts);
	if( www_curl_perform(ch) ) return NULL;
	return mem_borrowed(data);
}

char* www_header_get(const char* url, unsigned touts){
	__wcc CURL* ch = www_curl_new(url);
	__free uint8_t* data = MANY(uint8_t, 1024);
	curl_easy_setopt(ch, CURLOPT_HEADER, 1L);
    curl_easy_setopt(ch, CURLOPT_NOBODY, 1L);
	curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, www_curl_buffer_recv);
	curl_easy_setopt(ch, CURLOPT_WRITEDATA, &data);
	curl_easy_setopt(ch, CURLOPT_TIMEOUT, touts);
	dbg_warning("%s", url);
	if( www_curl_perform(ch) ) return NULL;
	data = mem_upsize(data, 1);
	data[mem_header(data)->len] = 0;
	return mem_borrowed(data);
}

