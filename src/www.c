#include <notstd/core.h>
#include <notstd/delay.h>
#include <notstd/str.h>

#include <gm/archive.h>

#include <omp.h>
#include <curl/curl.h>
#include <string.h>
#include <sys/wait.h>

#define __wcc __cleanup(www_curl_cleanup)

#define WWW_PREVENT_LONG_WAIT_SLOW_SERVER 30
#define WWW_ERROR_HTTP       10000
#define WWW_CURL_BUFFER_SIZE (MiB*4)
#define WWW_BUFFER_SIZE      (WWW_CURL_BUFFER_SIZE*2)

__thread unsigned wwwerrno;
__private CURL** tlcurl;
__private __thread int wwwgzstate;

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

unsigned www_errno(void){
	return wwwerrno;
}

unsigned www_connection_error(unsigned error){
	if( error < WWW_ERROR_HTTP ) return error;
	return 0;
}

unsigned www_http_error(unsigned error){
	if( error < WWW_ERROR_HTTP ) return 0;
	return error - WWW_ERROR_HTTP;
}

const char* www_str_error(unsigned error){
	if( error > WWW_ERROR_HTTP ) return www_errno_http(error - WWW_ERROR_HTTP);
	return curl_easy_strerror(error);
}

unsigned www_gzstate(void){
	return wwwgzstate;
}

__private size_t www_curl_buffer_recv(void* ptr, size_t size, size_t nmemb, void* userctx){
	void* ctx = *(void**)userctx;
	const size_t sizein = size * nmemb;
	uint8_t* data = mem_upsize(ctx, sizein);
	memcpy(data + mem_header(data)->len, ptr, sizein);
	mem_header(data)->len += sizein;
	*(void**)userctx = data;
	return sizein;
}

__private size_t www_curl_buffer_recv_decompress(void* ptr, size_t size, size_t nmemb, void* userctx){
	const size_t sizein = size * nmemb;
	if( wwwgzstate != 0 ){
		dbg_error("wrong internal state");
		if( wwwgzstate == -1 ){
			dbg_error("downloaded data but file it has already been decompressed");
			wwwgzstate = EBADE;
			errno = EBADE;
		}
		return 0;
	}
	switch( gzip_decompress_stream(userctx, ptr, sizein) ){
		default:
		case -1:
			wwwgzstate = errno;
			dbg_error("decompression fail: %m"); 
		return 0;
		case  0:
			//dbg_info("partial download %lu", sizein);
			return sizein;
		case  1:
			wwwgzstate = -1;
			dbg_info("download end");
		return sizein;
	}
}

//__private void www_curl_cleanup(void* ch){
//	curl_easy_cleanup(*(void**)ch);
//}

__private void www_curl_url(CURL* ch, const char* url){
	curl_easy_setopt(ch, CURLOPT_URL, url);
	if( !strncmp(url, "https", 5) )
		curl_easy_setopt(ch, CURLOPT_SSL_VERIFYPEER, 1L);
	else
		curl_easy_setopt(ch, CURLOPT_SSL_VERIFYPEER, 0L);
}

__private CURL* www_curl_new(void){
	CURL* ch = curl_easy_init();
	if( !ch ) die("curl init(%d): %s", errno, curl_easy_strerror(errno));
	curl_easy_setopt(ch, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(ch, CURLOPT_VERBOSE, 0L);
	return ch;
}

__private void www_curl_common_download_file(CURL* ch){
	curl_easy_setopt(ch, CURLOPT_FOLLOWLOCATION, 1L);
	//curl_easy_setopt(ch, CURLOPT_BUFFERSIZE, WWW_CURL_BUFFER_SIZE);
}

__private int www_curl_perform(CURL* ch, char** realurl){
	CURLcode res;
	res = curl_easy_perform(ch);
	if ( res != CURLE_OK && res != CURLE_FTP_COULDNT_RETR_FILE ){
		dbg_error("perform return %d: %s", res, curl_easy_strerror(res));
		wwwerrno = res;
		return -1;
	}
	if( realurl ){
		char* followurl = NULL;
		curl_easy_getinfo(ch, CURLINFO_EFFECTIVE_URL, &followurl);
		*realurl = str_dup(followurl, 0);
		dbg_info("realurl: %s", *realurl);
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

void www_begin(unsigned maxthr){
	curl_global_init(CURL_GLOBAL_DEFAULT);
	tlcurl = MANY(CURL*, maxthr);
	mem_header(tlcurl)->len = maxthr;
	for( unsigned i = 0; i < maxthr; ++i ){
		tlcurl[i] = www_curl_new();
		www_curl_common_download_file(tlcurl[i]);
	}
}

void www_end(void){
	mforeach(tlcurl, i){
		curl_easy_cleanup(tlcurl[i]);
	}
	mem_free(tlcurl);
	curl_global_cleanup();
}

void* www_download(const char* url, unsigned onlyheader, unsigned touts, char** realurl){
	dbg_info("'%s'", url);
	const unsigned tid = omp_get_thread_num();
	CURL* ch = tlcurl[tid];
	www_curl_url(ch, url);
	__free uint8_t* data = MANY(uint8_t, WWW_BUFFER_SIZE);
	curl_easy_setopt(ch, CURLOPT_WRITEDATA, &data);
	curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, www_curl_buffer_recv);
	if( onlyheader ){
		curl_easy_setopt(ch, CURLOPT_HEADER, 1L);
		curl_easy_setopt(ch, CURLOPT_NOBODY, 1L);
	}
	if( touts ) curl_easy_setopt(ch, CURLOPT_CONNECTTIMEOUT, touts);
	if( www_curl_perform(ch, realurl) ){
		dbg_error("curl perform");
		return NULL;
	}
	dbg_info("download and decompress successfull");
	return mem_borrowed(data);
}

void* www_download_gz(const char* url, unsigned onlyheader, unsigned touts, char** realurl){
	dbg_info("'%s'", url);
	const unsigned tid = omp_get_thread_num();
	wwwgzstate = 0;
	CURL* ch = tlcurl[tid];
	www_curl_url(ch, url);
	__free uint8_t* data = MANY(uint8_t, WWW_BUFFER_SIZE);
	curl_easy_setopt(ch, CURLOPT_WRITEDATA, &data);
	curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, www_curl_buffer_recv_decompress);
	if( onlyheader ){
		curl_easy_setopt(ch, CURLOPT_HEADER, 1L);
		curl_easy_setopt(ch, CURLOPT_NOBODY, 1L);
	}
	if( touts ) curl_easy_setopt(ch, CURLOPT_CONNECTTIMEOUT, touts);
	if( WWW_PREVENT_LONG_WAIT_SLOW_SERVER > 0 ) curl_easy_setopt(ch, CURLOPT_TIMEOUT, WWW_PREVENT_LONG_WAIT_SLOW_SERVER);
	dbg_info("start download");
	if( www_curl_perform(ch, realurl) ) return NULL;
	return mem_borrowed(data);
}

char* www_host_get(const char* url){
	__private const char* know[] = {
		"http://",
		"https://",
		"ftp://",
		"ftps://",
	};
	if( *url == '/' ) return NULL;
	for( unsigned i = 0; i < sizeof_vector(know); ++i ){
		if( !strncmp(url, know[i], strlen(know[i])) ){
			url += strlen(know[i]);
			break;
		}
	}
	const char* starthost = url;
	const char* endhost   = strchrnul(url,'/');
	if( endhost - starthost < 3 ) return NULL;
	return str_dup(starthost, endhost - starthost);
}

