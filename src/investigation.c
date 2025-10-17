#include <notstd/core.h>
#include <notstd/str.h>

#include <gm/investigation.h>
#include <gm/www.h>

extern char* REPO[2];

#define INVESTIGATE_ERROR     0x01
#define INVESTIGATE_OUTOFDATE 0x02
#define INVESTIGATE_ALL       0xFF

__private const char* repo_from_url(const char* url){
	const char* fname = strrchr(url, '/');
	if( !fname ) return NULL;
	++fname;
	const char* endname = strchr(fname, '.');
	if( !endname ) return NULL;
	for(unsigned i = 0; i < sizeof_vector(REPO); ++i ){
		if( !strncmp(REPO[i], fname, strlen(REPO[i])) ) return REPO[i];
	}
	return NULL;
}

__private int test_check_redirect_url(const char* url, const char* arch){
	const char* repo = repo_from_url(url);
	if( !repo ) return -1;
	__free char* endurl = str_printf("/%s/os/%s/%s.db", repo, arch, repo);
	unsigned urllen = strlen(url);
	unsigned partlen = strlen(endurl);
	if( urllen < partlen ) return -2;
	if( strcmp(&url[urllen-partlen], endurl) ) return -2;
	return 0;
}

__private int inv_mirror_iserr(mirror_s* mirror){
	return mirror->status == MIRROR_ERR || mirror->ping < 0 ? 1: 0;
}

__private void investigate_mirror(mirror_s* mirror, __unused mirror_s* local, unsigned mode){
	if( !inv_mirror_iserr(mirror) && !(mode & INVESTIGATE_OUTOFDATE) ) return;
	if( !inv_mirror_iserr(mirror) && !(mode & INVESTIGATE_ERROR) ) return;
	
	printf("server: '%s'\n", mirror->url);
	if( (mode & INVESTIGATE_ERROR) && mirror->isproxy ){
		printf("  redirect: '%s'\n", mirror->proxy);
		switch( test_check_redirect_url(mirror->proxy, mirror->arch) ){
			case -1: puts("  test url string: redirect url not contain repo name"); break;
			case -2: puts("  test url string: redirect url is malformed"); break;
			case  0: puts("  test url string: successfull"); break;
		}
	}
	if( (mode & INVESTIGATE_ERROR) ){
		switch( mirror->error ){
			case 0                  : break;
			case ERROR_GZIP         : puts("  error: unable to gzip file, probably corrupted file"); break;
			case ERROR_GZIP_DATA    : puts("  error: unable to gzip file, probably corrupted file or malicious redirection behavior"); break;
			case ERROR_TAR_MAGIC    : puts("  error: wrong magic tar value, probably corrupted file"); break;
			case ERROR_TAR_NOBLOCK  : puts("  error: tar aspected another block but not finding, probably connection interrupt download or corrupted file"); break;
			case ERROR_TAR_BLOCKEND : puts("  error: tar aspected end block but not finding, probably connection interrupt download or corrupted file"); break;
			case ERROR_TAR_CHECKSUM : puts("  error: fail tar checksum, probably corrupted file"); break;
			case ERROR_TAR_KV_ASSIGN: puts("  error: fail tar pax kv, probably corrupted file"); break;
			default: die("internal error, unmanaged %u error, please report this", mirror->error); break;
		}
		
		unsigned errconnection = www_connection_error(mirror->wwwerror);
		unsigned errhttp       = www_http_error(mirror->wwwerror);
		if( errconnection ){
			printf("  connection error %u: %s\n", errconnection, www_str_error(mirror->wwwerror)); 
			if( mirror->isproxy ){
				__free char* rh = www_host_get(mirror->proxy);
				if( rh ){
					if( www_ping(rh) < 0 ){
						printf("  ping proxy test: server is up but redirect server is down (%m)\n");
					}
					else{
						puts("  ping proxy test: server and redirect server is up");
					}
				}
				else{
					printf("  ping proxy test: fail to get host proxy: %s\n", mirror->proxy);
				}
			}
		}
		if( errhttp ){
			printf("  http error %u: %s\n", errhttp, www_str_error(mirror->wwwerror)); 
		}
		if( mirror->ping < 0 ){
			__free char* rh = www_host_get(mirror->url);
			if( rh ){
				if( www_ping(rh) < 0 ){
					printf("  ping test: %m\n");
				}
				else{
					puts("  ping test: now server is up");
				}
			}
			else{
				printf("  ping test: fail to get host from url: %s\n", mirror->url);
			}
		}
	}

	/*
	if( (mode & INVESTIGATE_OUTOFDATE) && mirror->status != MIRROR_ERR && mirror->outofdate && local ){
		const unsigned repocount = sizeof_vector(REPO);
		for( unsigned ir = 0; ir < repocount; ++ir ){
			unsigned const dbcount = mem_header(local->repo[ir].db)->len;
			unsigned linestart = printf("  outofdate.%-5s: ", REPO[ir]);
			unsigned linelen = 1;
			for( unsigned i = 0; i < dbcount; ++i){
				pkgdesc_s* tpk = mem_bsearch(mirror->repo[ir].db, &local->repo[ir].db[i], pkgname_cmp);
				if( tpk ){
					int ret = pkg_vercmp(local->repo[ir].db[i].version, tpk->version);
					if( ret == 1 ){
						fputs(tpk->name, stdout);
						putchar(',');
						putchar(' ');
						linelen += strlen(tpk->name);
						if( linelen > 80 ){
							linelen = 0;
							putchar('\n');
							for( unsigned pi = 0; pi < linestart; ++pi ) putchar(' ');
						}
					}
				}
			}
			if( linelen ) putchar('\n');
		}
	}
	*/
	putchar('\n');
}

__private unsigned cast_mode(const char* mode){
	//TODO add mode for investigate outofdate package, repo[].db now is free and can't use
	if( !strcmp(mode, "outofdate") ) die("this is a todo feature, new version use less ram need to change code for reuse this options, please wait");

	static char*    invname[] = { "error"          , "outofdate"          , "all"           };
	static unsigned inval[]   = { INVESTIGATE_ERROR, INVESTIGATE_OUTOFDATE, INVESTIGATE_ERROR /*INVESTIGATE_ALL*/ };
	for( unsigned i = 0; i < sizeof_vector(invname); ++i ){
		if( !strcmp(invname[i], mode) ) return inval[i];
	}
	die("unknow investigation mode: %s", mode);
}

void investigate_mirrors(mirror_s* mirrors, option_s* oinv){
	unsigned mode = 0;
	for( unsigned i = 0; i < oinv->set; ++i ){
		mode |= cast_mode(oinv->value[i].str);
	}
	
	mirror_s* local = NULL;
	const unsigned count = m_header(mirrors)->len;
	dbg_info("investigate on %u mirrors", count);
	for( unsigned i = 0; i < count; ++i ){
		if( mirrors[i].status == MIRROR_COMPARE ){
			local = &mirrors[i];	
			break;
		}
	}

	long ic = www_ping(IP_TEST_INTERNET_CONNECTION);
	if( ic > 0 && ic < 2000000 ){
		printf("internet connection: %.1fms\n", ic/1000.0 );
	}else{
		printf("internet connection: error\n");
	}

	for( unsigned i = 0; i < count; ++i ){
		investigate_mirror(&mirrors[i], local, mode);
	}
}
