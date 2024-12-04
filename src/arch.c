#define _GNU_SOURCE
#include <notstd/core.h>
#include <notstd/str.h>
#include <notstd/delay.h>

#include <gm/arch.h>
#include <gm/archive.h>
#include <gm/www.h>

#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#define PACMAN_MIRRORLIST "/etc/pacman.d/mirrorlist"
#define PACMAN_LOCAL_DB   "/var/lib/pacman/sync"

__private const char* SORTNAME[FIELD_COUNT] = {
	"outofdate",
	"uptodate",
	"morerecent",
	"sync",
	"retry",
	"speed"
};

__private char* REPO[] = { "core", "extra" };

__private mirror_s* mirror_ctor(mirror_s* mirror, char* url, const char* arch, char* country){
	memset(mirror, 0, sizeof(mirror_s));
	mirror->url     = url;
	mirror->arch    = arch;
	mirror->status  = MIRROR_UNKNOW;
	mirror->country = country;
	dbg_info("'%s'", url);
	return mirror;
}

__private char* load_file(const char* fname, int exists){
	dbg_info("loading %s", fname);
	int fd = open(fname, O_RDONLY);
	if( fd < 0 ){
		if( exists ) die("unable to open file: %s, error: %s", fname, strerror(errno));
		return NULL;
	}
	char* buf = MANY(char, 4096);
	ssize_t nr;
	while( (nr=read(fd, &buf[mem_header(buf)->len], mem_available(buf))) > 0 ){
		mem_header(buf)->len += nr;
		buf = mem_upsize(buf, 4096);
	}
	close(fd);
	if( nr < 0 ) die("unable to read file: %s, error: %s", fname, strerror(errno));
	buf = mem_fit(buf);
	return buf;
}

__private const char* skip_h(const char* data, const char* end){
	while( data < end && (*data == ' ' || *data == '\t') ) ++data;
	return data;
}

__private int section(const char* secname, const char* data, const char* end){
	data = skip_h(data, end);
	if( *data++ != '%' ) return 0;
	const size_t len = strlen(secname);
	if( strncmp(data, secname, len) ) return 0;
	data += len;
	if( *data != '%' ) return 0;
	return 1;
}

__private const char* next_line(const char* data, const char* end){
	while( data < end && *data != '\n' ) ++data;
	if( data < end ) ++data;
	return data;
}

__private void read_str(char dst[static NAME_MAX], const char* data, const char* end){
	unsigned const max = NAME_MAX - 1;
	unsigned i;
	data = skip_h(data, end);
	for( i = 0; i < max && data < end && *data != '\n'; ++i, ++data ){
		dst[i] = *data;
	}
	dst[i] = 0;
}

__private void pkgdesc_parse(pkgdesc_s* out, const char* data, size_t len){
	const char* end = data + len;

	out->filename[0] = 0;
	out->name[0]     = 0;
	out->version[0]  = 0;
	out->lastsync = 0;

	int nval = 3;	
	while( nval && data < end ){
		if( *skip_h(data, end) != '%' ){
			data = next_line(data, end);
			continue;
		}

		if( section("FILENAME", data, end) ){
			data = next_line(data, end);
			read_str(out->filename, data, end);
			data = next_line(data, end);
			--nval;
		}
		else if( section("NAME", data, end) ){
			data = next_line(data, end);
			read_str(out->name, data, end);
			data = next_line(data, end);
			--nval;
		}
		else if( section("VERSION", data, end) ){
			data = next_line(data, end);
			read_str(out->version, data, end);
			data = next_line(data, end);
			--nval;
		}
		else{
			data = next_line(data, end);
		}
	}
}

__private int pkg_vercmp(const char *a, const char *b){
    const char *pa = a, *pb = b;
    int r = 0;

    while (*pa || *pb) {
        if (isdigit(*pa) || isdigit(*pb)) {
            long la = 0, lb = 0;
            while (*pa == '0') {
                pa++;
            }
            while (*pb == '0') {
                pb++;
            }
            while (isdigit(*pa)) {
                la = la * 10 + (*pa - '0');
                pa++;
            }
            while (isdigit(*pb)) {
                lb = lb * 10 + (*pb - '0');
                pb++;
            }
            if (la < lb) {
                return -1;
            } else if (la > lb) {
                return 1;
            }
        } else if (*pa && *pb && isalpha(*pa) && isalpha(*pb)) {
            r = tolower((unsigned char)*pa) - tolower((unsigned char)*pb);
            if (r != 0) {
                return r;
            }
            pa++;
            pb++;
        } else {
            char ca = *pa ? *pa : 0;
            char cb = *pb ? *pb : 0;

            if (ca == '-' || ca == '_') {
                ca = '.';
            }
            if (cb == '-' || cb == '_') {
                cb = '.';
            }

            if (ca != cb) {
                return ca - cb;
            }
            if (ca) {
                pa++;
            }
            if (cb) {
                pb++;
            }
        }
    }

    return 0;
}

__private pkgdesc_s* generate_db(void* tarbuf){
	tar_s tar;
	tar_mopen(&tar, tarbuf);
	errno = 0;
	tarent_s* ent;
	pkgdesc_s* db = MANY(pkgdesc_s, 100);
	while( (ent=tar_next(&tar)) ){
		if( ent->type == TAR_FILE ){
			db = mem_upsize(db, 1);
			pkgdesc_parse(&db[mem_header(db)->len++], ent->data, ent->size);
			mem_free(ent);
		}
	}
	if( errno ){
		mem_free(db);
		return NULL;
	}
	tar_close(&tar);
	return db;
}

__private int pkgname_cmp(const void* a, const void* b){
	const pkgdesc_s* da = a;
	const pkgdesc_s* db = b;
	return strcmp(da->name, db->name);
}

__private int lsname_cmp(const void* a, const void* b){
	const char** da = (const char**)a;
	const char** db = (const char**)b;
	return strcmp(*da, *db);
}

//I can only imagine how portable and safe this is
__private char** https_ls_parse(const char* str){
	char** ls = MANY(char*, 64);
	while( (str=strstr(str, "<a href=\"")) ){
		str += 9;
		const char* end = strstr(str, "\">");
		if( !end ) return ls;
		if( strncmp(end-4, ".zst", 4) ){
			str = end;
			continue;
		}
		ls = mem_upsize(ls, 1);
		ls[mem_header(ls)->len++] = str_dup(str, end-str);
		str = end;
	}
	return ls;
}

__private void* get_tar_zst(mirror_s* mirror, const char* repo, const unsigned tos){
	if( mirror->url[0] == '/' ){
		__free char* url = str_printf("%s/%s.db", mirror->url, repo);
		return load_file(url, 1);
	}
	else{
		__free char* url = str_printf("%s/%s/os/%s/%s.db", mirror->url, repo, mirror->arch, repo);
		return www_mdownload_retry(url, tos, DOWNLOAD_RETRY, DOWNLOAD_WAIT);
	}
}

__private char* get_mirror_ls(mirror_s* mirror, const char* repo, const unsigned tos){
	if( mirror->url[0] == '/' ) return NULL;
	__free char* url = str_printf("%s/%s/os/%s/", mirror->url, repo, mirror->arch);
	return www_mdownload_retry(url, tos, DOWNLOAD_RETRY, DOWNLOAD_WAIT);
}

__private void mirror_update(mirror_s* mirror, const unsigned tos){
	const unsigned repocount = sizeof_vector(REPO);
	dbg_info("update %s", mirror->url);
	for( unsigned ir = 0; ir < repocount; ++ir ){
		dbg_info("\t %s", REPO[ir]);
		__free void* tarzstd = get_tar_zst(mirror, REPO[ir], tos);
		if( !tarzstd ){
			dbg_error("unable to get remote mirror: %s", mirror->url);
			mirror->status = MIRROR_ERR;
			return;
		}

		__free void* tarbuf = gzip_decompress(tarzstd);
		if( !tarbuf ){
			dbg_error("decompress zstd archive from mirror: %s", mirror->url);
			mirror->status = MIRROR_ERR;
			return;
		}

		if( !(mirror->repo[ir].db = generate_db(tarbuf)) ){
			dbg_error("untar archive from mirror: %s", mirror->url);
			mirror->status = MIRROR_ERR;
			return;
		}

		mem_qsort(mirror->repo[ir].db, pkgname_cmp);
		mirror->rfield[FIELD_TOTAL] += mem_header(mirror->repo[ir].db)->len;

		__free char* rls = get_mirror_ls(mirror, REPO[ir], tos);
		if( rls ){
			mirror->repo[ir].ls = https_ls_parse(rls);
			mem_qsort(mirror->repo[ir].ls, lsname_cmp);
		}
		else{
			dbg_warning("mirror %s not provide ls", mirror->url);
		}
	}
}

__private void progress_begin(const char* desc, unsigned count){
	fprintf(stderr, "[%5.1f%%] %s", 0 * 100.0 / count, desc);
	fflush(stderr);
}

__private void progress_refresh(const char* desc, unsigned value, unsigned count){
	char out[512];
	unsigned n = sprintf(out, "\r[%5.1f%%] %s", value * 100.0 / count, desc);
	write(2, out, n);
}

__private void progress_end(const char* desc){
	fprintf(stderr, "\r[100.0%%] %s\n", desc);
	fflush(stderr);
}

void mirrors_update(mirror_s* mirrors, const int progress, const unsigned ndownload, const unsigned tos){
	dbg_info("");
	const unsigned count = mem_header(mirrors)->len;
	__atomic unsigned pvalue = 0;
	
	if( progress ) progress_begin("mirrors updates", count);

	__paralleft(ndownload)
	for( unsigned i = 0; i < count; ++i){
		mirror_update(&mirrors[i], tos);
		if( progress ) progress_refresh("mirrors updates", ++pvalue, count);
	}

	if( progress ) progress_end("mirrors updates");
}

char* mirror_loading(const char* fname, const unsigned tos){
	char* buf = fname ? load_file(fname, 1) :  www_mdownload_retry("https://archlinux.org/mirrorlist/all/", tos, DOWNLOAD_RETRY, DOWNLOAD_WAIT);
	if( !buf ) die("unable to load mirrorlist");
	buf = mem_nullterm(buf);
	return buf;
}

__private const char* find_country(const char* str, const char* country){
	const size_t len = strlen(country);
	while( (str=strstr(str, "## ")) ){
		str += 3;
		if( !strncmp(str, country, len) ){
			str += len;
			while( *str && *str == ' ') ++str;
			if( *str == '\n' ) return str + 1;
		}
	}
	die("country %s not exists", country);
}

__private int check_type(const char* url, unsigned type){
	if( (type & MIRROR_TYPE_HTTP ) && !strncmp(url, "http:" , 5) ) return MIRROR_TYPE_HTTP;
	if( (type & MIRROR_TYPE_HTTPS) && !strncmp(url, "https:", 5) ) return MIRROR_TYPE_HTTPS;
	return 0;
}

__private char* server_url(const char** pline, int uncommented, int restrictcountry, unsigned type){
	const char* line = *pline;

	while( *line ){
		while( *line && *line == ' ' ) ++line;
		if( !*line ) return NULL;
		if( *line == '\n' ){ ++line; continue; }

		if( restrictcountry && !strncmp(line, "## ", 3) ) break;
		
		if( uncommented && *line == '#' ){ line = str_next_line(line); continue; }
		if( *line == '#' ) ++line;

		while( *line && *line == ' ' ) ++line;
		if( strncmp(line, "Server", 6) ){ line = str_next_line(line); continue; }
		line += 6;
		while( *line && *line == ' ' ) ++line;
		if( *line != '=' ){ line = str_next_line(line); continue; }
		++line;
		while( *line && *line == ' ' ) ++line;
		const char* url = line;
		const char* endurl = strpbrk(url, "$\n");
		if( !endurl ){ line = str_next_line(line); continue; }
		if( *endurl == '\n' ){ ++line; continue; }
		--endurl;
		*pline = str_next_line(endurl);
		if( !check_type(url, type) ){ line = str_next_line(endurl); continue; }
		return str_dup(url, endurl - url);
	}
	return NULL;
}

__private char* back_start_country_mark(const char* from, const char* begin){
	while( from > begin ){
		if( *from == '#' ){
			--from;
			if( *from == '#' ){
				from +=3;
				const char* end = strchrnul(from, '\n');
				while( end[-1] == ' ' ) --end;
				return str_dup(from, end-from);
			}
		}
		--from;
	}
	return "UserDefined";
}

__private char* server_find_country(const char* url, const char* mirrorlist){
	const char* loc = strstr(mirrorlist, url);
	if( !loc ) return "UserDefined";
	return back_start_country_mark(loc, mirrorlist);
}

mirror_s* mirrors_country(mirror_s* mirrors, const char* mirrorlist, const char* safemirrorlist, const char* country, const char* arch, int uncommented, unsigned type){
	char* url;
	const char* fromcountry = country ? find_country(mirrorlist, country) : mirrorlist;

	if( !mirrors ){
		mirrors = MANY(mirror_s, 10);
		__free char* localmirror = load_file(PACMAN_MIRRORLIST, 1);
		localmirror = mem_nullterm(localmirror);

		url = server_url((const char**)&localmirror, 1, 0, type);
		if( !url ) url = PACMAN_LOCAL_DB;
		const unsigned id = mem_header(mirrors)->len++;
		mirror_ctor(&mirrors[id], url, arch, server_find_country(url, safemirrorlist));
		mirrors[id].status  = MIRROR_LOCAL;
	}

	while( (url=server_url(&fromcountry, uncommented, country ? 1 : 0, type)) ){
		mirrors = mem_upsize(mirrors, 1);
		const unsigned id = mem_header(mirrors)->len++;
		char* fcountry = country ? (char*)country : server_find_country(url, safemirrorlist);
		mirror_ctor(&mirrors[id], url, arch, fcountry);
	}

	return mirrors;
}

void country_list(const char* mirrorlist){
	while( (mirrorlist=strstr(mirrorlist, "## ")) ){
		mirrorlist += 3;
		const char* nl = strchrnul(mirrorlist, '\n');
		printf("%.*s\n", (int)(nl-mirrorlist), mirrorlist);
		mirrorlist = nl;
	}
}

__private int lsname_cmp2(const void* a, const void* b){
	const char* da = (const char*)a;
	const char** db = (const char**)b;
	return strcmp(da, *db);
}

__private void mirror_cmp_db(mirror_s* local, mirror_s* test){
	const unsigned repocount = sizeof_vector(REPO);
	for( unsigned ir = 0; ir < repocount; ++ir ){
		mforeach(local->repo[ir].db, i){
			pkgdesc_s* tpk = mem_bsearch(test->repo[ir].db, &local->repo[ir].db[i], pkgname_cmp);
			if( tpk ){
				int ret = pkg_vercmp(local->repo[ir].db[i].version, tpk->version);
				switch( ret ){
					case -1: ++test->rfield[FIELD_MORERECENT]; break;
					case  1: ++test->rfield[FIELD_OUTOFDATE]; break;
					case  0: ++test->rfield[FIELD_UPTODATE]; break;
				}
				if( test->repo[ir].ls && mem_bsearch(test->repo[ir].ls, tpk->filename, lsname_cmp2) ){
					++test->rfield[FIELD_SYNC];
				}
			}
			/*
			 * i have remoded noexists because i not know if not exists because is new version or old version 
			 * ++test->rfield[FIELD_NOEXISTS];
			*/

		}
	}

	/* now new version count noexists */
	unsigned managed = 0;
	for( unsigned i = 0; i < FIELD_SYNC; ++i ){
		managed += test->rfield[i];
	}
	if( managed < test->rfield[FIELD_TOTAL] ) test->rfield[FIELD_MORERECENT] += test->rfield[FIELD_TOTAL] - managed;
}

void mirrors_cmp_db(mirror_s* mirrors, const int progress){
	dbg_info("");
	mirror_s* local = NULL;
	const unsigned count = mem_header(mirrors)->len;
	if( count < 2 ) die("impossible to compare less than 2 mirrors");
	for( unsigned i = 0; i < count; ++i ){
		if( mirrors[i].status == MIRROR_LOCAL ){
			local = &mirrors[i];
			local->rfield[FIELD_UPTODATE] = local->rfield[FIELD_TOTAL];
			const unsigned repocount = sizeof_vector(REPO);
			for( unsigned ir = 0; ir < repocount; ++ir ){
				if( local->repo[ir].ls ){
					mforeach(local->repo[ir].db, i){
						if( mem_bsearch(local->repo[ir].ls, local->repo[ir].db[i].filename, lsname_cmp2) ) ++local->rfield[FIELD_SYNC];
					}
				}
			}
			break;
		}
	}
	if( !local ) die("internal error, not find local db, report this message");

	if( progress ) progress_begin("mirrors db compare", count);
	
	for( unsigned i = 0; i < count; ++i ){
		if( mirrors[i].status == MIRROR_UNKNOW ){
			mirror_cmp_db(local, &mirrors[i]);
		}
		if( progress ) progress_refresh("mirrors db compare", i, count);
	}
	
	if( progress ) progress_end("mirrors db compare");
	dbg_info("end compare mirror database");
}

__private field_e sortmode[FIELD_COUNT];
__private unsigned sortcount;

__private int sort_real_cmp(const mirror_s* a, const mirror_s* b, const field_e sort){
	static int ascdsc[FIELD_COUNT] = { 1, 0, 0, 0, 1, 0, 0 };
	iassert(sort < FIELD_TOTAL);
	if( sort == FIELD_VIRTUAL_SPEED ){
		if( a->speed > b->speed ) return -1;
		if( a->speed < b->speed ) return 1;
		return 0;
	}
	if( ascdsc[sort] ){
		return a->rfield[sort] - b->rfield[sort];
	}
	return b->rfield[sort] - a->rfield[sort];
}

__private int sort_cmp(const void* a, const void* b){
	unsigned count = sortcount;
	for( unsigned i = 0; i < count; ++i ){
		int ret = sort_real_cmp(a, b, sortmode[i]);
		if( ret ) return ret;
	}
	return 0;
}

__private unsigned sort_name_to_id(const char* name){
	for( unsigned i = 0; i < FIELD_TOTAL; ++i ){
		if( !strcmp(SORTNAME[i], name) ){
			return i;
		}
	}
	die("unknow sort mode: %s", name);
}

void add_sort_mode(const char* mode){
	sortmode[sortcount++] = sort_name_to_id(mode);
}

void mirrors_sort(mirror_s* mirrors){
	if( !sortcount ) die("need to set any or more valid sort modes");
	mem_qsort(mirrors, sort_cmp);
}

__private void mirror_speed(mirror_s* mirror, const char* arch){
	pkgdesc_s  find = { .name = "chromium" };
	pkgdesc_s* pk = mem_bsearch(mirror->repo[1].db, &find, pkgname_cmp);
	if( !pk ){
		dbg_error("unable to benchmark mirror, not find chromium test package");
		return;
	}

	__free char* url = str_printf("%s/extra/os/%s/%s", mirror->url, arch, pk->filename);
	unsigned retry = DOWNLOAD_RETRY;
	while( retry-->0 ){
		delay_t start = time_sec();
		__free void* buf = www_mdownload(url, 0);
		delay_t stop  = time_sec();
		if( !buf ){
			++mirror->rfield[FIELD_RETRY];
			delay_ms(DOWNLOAD_WAIT);
			continue;
		}
		unsigned size = mem_header(buf)->len;
		mirror->speed = (size / (1024.0*1024.0)) / (stop-start);
		break;
	}
}

void mirrors_speed(mirror_s* mirrors, const char* arch, int progress){
	const unsigned count = mem_header(mirrors)->len;
	if( progress ) progress_begin("mirrors speed", count);
		
	mforeach(mirrors, i){
		if( mirrors[i].status != MIRROR_ERR ){
			mirror_speed(&mirrors[i], arch );
		}
		if( progress ) progress_refresh("mirrors speed", i, count);
	}

	if( progress ) progress_end("mirrors speed");
}

