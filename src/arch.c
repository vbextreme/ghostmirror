#define _GNU_SOURCE
#include <notstd/core.h>
#include <notstd/str.h>

#include <gm/arch.h>
#include <gm/archive.h>
#include <gm/www.h>

#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#define DOWNLOAD_RETRY 3
#define DOWNLOAD_WAIT  1000

__private char* REPO[] = { "core", "extra" };

__private mirror_s* mirror_ctor(mirror_s* mirror, char* url, const char* arch){
	memset(mirror, 0, sizeof(mirror_s));
	mirror->url    = url;
	mirror->arch   = arch;
	mirror->status = MIRROR_UNKNOW;
	dbg_info("'%s'", url);
	return mirror;
}

__private void* load_file(const char* fname){
	dbg_info("loading %s", fname);
	int fd = open(fname, O_RDONLY);
	if( fd < 0 ) die("unable to open file: %s, error: %s", fname, strerror(errno));
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
		char* cps = str_dup(str, end-str);
		ls = mem_push(ls, &cps);
		str = end;
	}
	return ls;
}

__private void* get_tar_zst(mirror_s* mirror, const char* repo, const unsigned tos){
	if( mirror->url[0] == '/' ){
		__free char* url = str_printf("%s/%s.db", mirror->url, repo);
		return load_file(url);
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
		mirror->totalpkg += mem_header(mirror->repo[ir].db)->len;

		__free char* rls = get_mirror_ls(mirror, REPO[ir], tos);
		if( rls ){
			mirror->repo[ir].ls = https_ls_parse(rls);
			mem_qsort(mirror->repo[ir].ls, (cmp_f)strcmp);
		}
		else{
			dbg_warning("mirror %s not provide ls", mirror->url);
		}
	}
}

void mirrors_update(mirror_s* mirrors, const int showprogress, const unsigned ndownload, const unsigned tos){
	dbg_info("");
	const unsigned count = mem_header(mirrors)->len;
	__atomic unsigned progress = 0;
	
	if( showprogress ){
		fprintf(stderr, "[%03u/%03u] mirror updates", progress, count);
		fflush(stderr);
	}

	__paralleft(ndownload)
	for( unsigned i = 0; i < count; ++i){
		mirror_update(&mirrors[i], tos);
		if( showprogress ){
			++progress;
			char out[512];
			unsigned n = sprintf(out, "\r[%03u/%03u] mirror updates", progress, count);
			write(2, out, n);
		}
	}

	if( showprogress ){
		fputc('\n', stderr);
		fflush(stderr);
	}
}

char* mirror_loading(const char* fname, const unsigned tos){
	char* buf = fname ? load_file(fname) :  www_mdownload_retry("https://archlinux.org/mirrorlist/all/", tos, DOWNLOAD_RETRY, DOWNLOAD_WAIT);
	if( !buf ) die("unable to load mirrorlist");
	buf = mem_upsize(buf,1);
	buf[mem_header(buf)->len] = 0;
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

__private const char* skip_n(const char* str){
	str = strchrnul(str, '\n');
	if( *str ) ++str;
	return str;
}

__private char* server_url(const char** pline, int uncommented, int restrictcountry){
	const char* line = *pline;

	while( *line ){
		while( *line && *line == ' ' ) ++line;
		if( !*line ) return NULL;
		if( *line == '\n' ){ ++line; continue; }

		if( restrictcountry && !strncmp(line, "## ", 3) ) break;
		
		if( uncommented && *line == '#' ){ line = skip_n(line); continue; }
		if( *line == '#' ) ++line;

		while( *line && *line == ' ' ) ++line;
		if( strncmp(line, "Server", 6) ){ line = skip_n(line); continue; }
		line += 6;
		while( *line && *line == ' ' ) ++line;
		if( *line != '=' ){ line = skip_n(line); continue; }
		++line;
		while( *line && *line == ' ' ) ++line;
		const char* url = line;
		const char* endurl = strpbrk(url, "$\n");
		if( !endurl ){ line = skip_n(line); continue; }
		if( *endurl == '\n' ){ ++line; continue; }
		--endurl;
		*pline = skip_n(endurl);
		return str_dup(url, endurl - url);
	}
	return NULL;
}

mirror_s* mirrors_country(mirror_s* mirrors, const char* mirrorlist, const char* country, const char* arch, int uncommented){
	char* url;
	const char* fromcountry = country ? find_country(mirrorlist, country) : mirrorlist;

	if( !mirrors ){
		mirrors = MANY(mirror_s, 10);
		const unsigned id = mem_header(mirrors)->len++;
		mirror_ctor(&mirrors[id], "/var/lib/pacman/sync", arch);
		mirrors[id].status = MIRROR_LOCAL;
	}

	while( (url=server_url(&fromcountry, uncommented, country ? 1 : 0)) ){
		mirrors = mem_upsize(mirrors, 1);
		const unsigned id = mem_header(mirrors)->len++;
		mirror_ctor(&mirrors[id], url, arch);
	}

	return mirrors;
}

__private void mirror_cmp_db(mirror_s* local, mirror_s* test){
	const unsigned repocount = sizeof_vector(REPO);
	for( unsigned ir = 0; ir < repocount; ++ir ){
		mforeach(local->repo[ir].db, i){
			pkgdesc_s* tpk = mem_bsearch(test->repo[ir].db, &local->repo[ir].db[i], pkgname_cmp);
			if( !tpk ){
				++test->notexistspkg;
			}
			else{
				int ret = pkg_vercmp(local->repo[ir].db[i].version, tpk->version);
				switch( ret ){
					case -1: ++test->syncdatepkg; break;
					case  1: ++test->outofdatepkg; break;
					case  0: ++test->uptodatepkg; break;
				}
				if( test->repo[ir].ls &&  mem_bsearch(test->repo[ir].ls, tpk->filename, (cmp_f)strcmp) ){
					++test->checked;
					dbg_info("CHECKED %u", test->checked);
				}
			}
		}
	}
	
	unsigned managed = test->syncdatepkg + test->outofdatepkg + test->uptodatepkg + test->notexistspkg;
	if( managed < test->totalpkg ) test->extrapkg = test->totalpkg - managed;
}

void mirrors_cmp_db(mirror_s* mirrors, const int progress){
	dbg_info("");
	mirror_s* local = NULL;
	const unsigned count = mem_header(mirrors)->len;
	if( count < 2 ) die("impossible to compare less than 2 mirrors");
	for( unsigned i = 0; i < count; ++i ){
		if( mirrors[i].status == MIRROR_LOCAL ){
			local = &mirrors[i];
			break;
		}
	}
	if( !local ) die("internal error, not find local db, report this message");

	if( progress ){
		fprintf(stderr, "[%.1f] mirror compare", 0 * 100.0 / count);
		fflush(stderr);
	}
	for( unsigned i = 0; i < count; ++i ){
		if( mirrors[i].status == MIRROR_UNKNOW ){
			mirror_cmp_db(local, &mirrors[i]);
		}
		if( progress ){
			char out[512];
			unsigned n = sprintf(out, "\r[%.1f] mirror compare", i * 100.0 / count);
			write(2, out, n);
		}
	}
	if( progress ){
		fputc('\n', stderr);
		fflush(stderr);
	}
	dbg_info("end compare mirror database");
}

