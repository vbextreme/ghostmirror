#include <notstd/core.h>
#include <notstd/str.h>

#include <gm/investigation.h>
#include <gm/www.h>

extern char* REPO[2];

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

__private void investigate_mirror(mirror_s* mirror, mirror_s* local){
	if( mirror->status != MIRROR_ERR && mirror->outofdate == 0 ) return;
	
	printf("server: '%s'\n", mirror->url);
	if( mirror->isproxy ){
		printf("  redirect: '%s'\n", mirror->proxy);
	}
	if( mirror->status == MIRROR_ERR ){
		puts("  state: error");
		unsigned errconnection = www_connection_error(mirror->wwwerror);
		unsigned errhttp       = www_http_error(mirror->wwwerror);
		if( errconnection ){
			printf("  connection error %u: %s\n", errconnection, www_str_error(mirror->wwwerror)); 
			if( mirror->isproxy ){
				if( www_ping(mirror->proxy) < 0 )
					puts("    ping proxy test: server is up but redirect server is down");
				else
					puts("    ping proxy test: server and redirect server is up");
			}
		}
		else if( errhttp ){
			printf("  http error %u: %s\n", errhttp, www_str_error(mirror->wwwerror)); 
			if( mirror->isproxy ){
				switch( test_check_redirect_url(mirror->proxy, mirror->arch) ){
					case -1: puts("    test url: redirect url not contain repo name"); break;
					case -2: puts("    test url: redirect url is malformed"); break;
					case  0: puts("    test url: successfull"); break;
				}
			}
		}
		else if( mirror->ping < 0 ){
			puts("  ping error: probably server is down"); 
		}
		else{
			puts("  error: strange behaviours, how have set error if not able to find where error is setted?");
		}
	}
	else{
		puts("  state: successfull");
	}

	if( mirror->status != MIRROR_ERR && mirror->outofdate ){
		const unsigned repocount = sizeof_vector(REPO);
		for( unsigned ir = 0; ir < repocount; ++ir ){
			unsigned const dbcount = mem_header(local->repo[ir].db)->len;
			unsigned linestart = printf("  outofdate.%-5s: ", REPO[ir]);
			unsigned linelen = 0;
			for( unsigned i = 0; i < dbcount; ++i){
				pkgdesc_s* tpk = mem_bsearch(mirror->repo[ir].db, &local->repo[ir].db[i], pkgname_cmp);
				if( tpk ){
					int ret = pkg_vercmp(local->repo[ir].db[i].version, tpk->version);
					if( ret == 1 ){
						fputs(tpk->name, stdout);
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
	putchar('\n');
}

void investigate_mirrors(mirror_s* mirrors){
	mirror_s* local = NULL;
	const unsigned count = mem_header(mirrors)->len;
	for( unsigned i = 0; i < count; ++i ){
		if( mirrors[i].status == MIRROR_LOCAL ){
			local = &mirrors[i];	
			break;
		}
	}
	if( !local ) die("internal error, not find local mirror");
	for( unsigned i = 0; i < count; ++i ){
		investigate_mirror(&mirrors[i], local);
	}
}
