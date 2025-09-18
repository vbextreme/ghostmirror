#define _GNU_SOURCE
#include <notstd/core.h>
#include <notstd/str.h>
#include <notstd/delay.h>

#include <gm/arch.h>
#include <gm/archive.h>
#include <gm/www.h>
#include <gm/systemd.h>
#include <gm/inutility.h>

#include <omp.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

#define SORT_MAX 32
__private const char* SORTNAME[] = {
	"country",
	"mirror",
	"proxy",
	"state",
	"outofdate",
	"uptodate",
	"morerecent",
	"retry",
	"speed",
	"ping",
	"estimated"
};
__private unsigned SORTMODE[SORT_MAX];
__private unsigned SORTCOUNT;


char* REPO[2] = { "core", "extra" };

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
		if( exists ) die("unable to open file: %s, error: %m", fname);
		return NULL;
	}
	char* buf = MANY(char, 4096);
	ssize_t nr;
	while( (nr=read(fd, &buf[mem_header(buf)->len], mem_available(buf))) > 0 ){
		mem_header(buf)->len += nr;
		buf = mem_upsize(buf, 4096);
	}
	close(fd);
	if( nr < 0 ) die("unable to read file: %s, error: %m", fname);
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

/**
 * Split EVR into epoch, version, and release components.
 * @param evr		[epoch:]version[-release] string
 * @retval *ep		pointer to epoch
 * @retval *vp		pointer to version
 * @retval *rp		pointer to release
 */
__private void parse_evr(char *evr, const char **ep, const char **vp, const char **rp){
	const char *epoch;
	const char *version;
	const char *release;
	char *s, *se;
	
	s = evr;
	/* s points to epoch terminator */
	while( *s && isdigit(*s) ) ++s;
	/* se points to version terminator */
	se = strrchr(s, '-');
	
	if( *s == ':' ){
		epoch = evr;
		*s++ = '\0';
		version = s;
		if( !*epoch ) epoch = "0";
	}
	else{
		/* different from RPM- always assume 0 epoch */
		epoch = "0";
		version = evr;
	}

	if( se ) *se++ = '\0';
	release = se;

	if( ep ) *ep = epoch;
	if( vp ) *vp = version;
	if( rp ) *rp = release;
}

/**
 * Compare alpha and numeric segments of two versions.
 * return 1: a is newer than b
 *        0: a and b are the same version
 *       -1: b is newer than a
 */
__private int rpmvercmp(const char *a, const char *b){
	char oldch1, oldch2;
	char str1[512], str2[512];
	char *ptr1, *ptr2;
	char *one, *two;
	int rc;
	int isnum;
	int ret = 0;

	/* easy comparison to see if versions are identical */
	if( strcmp(a, b) == 0 ) return 0;
	strcpy(str1, a);
	strcpy(str2, b);
	
	one = ptr1 = str1;
	two = ptr2 = str2;

	/* loop through each version segment of str1 and str2 and compare them */
	while( *one && *two ){
		while( *one && !isalnum((int)*one) ) one++;
		while( *two && !isalnum((int)*two) ) two++;
		
		/* If we ran to the end of either, we are finished with the loop */
		if( !(*one && *two) ) break;
		
		/* If the separator lengths were different, we are also finished */
		if( (one - ptr1) != (two - ptr2) ){
			ret = (one - ptr1) < (two - ptr2) ? -1 : 1;
			return ret;
		}
		
		ptr1 = one;
		ptr2 = two;
		
		/* grab first completely alpha or completely numeric segment */
		/* leave one and two pointing to the start of the alpha or numeric */
		/* segment and walk ptr1 and ptr2 to end of segment */
		if( isdigit((int)*ptr1) ){
			while( *ptr1 && isdigit((int)*ptr1) ) ptr1++;
			while( *ptr2 && isdigit((int)*ptr2) ) ptr2++;
			isnum = 1;
		} 
		else{
			while (*ptr1 && isalpha((int)*ptr1)) ptr1++;
			while (*ptr2 && isalpha((int)*ptr2)) ptr2++;
			isnum = 0;
		}
		
		/* save character at the end of the alpha or numeric segment */
		/* so that they can be restored after the comparison */
		oldch1 = *ptr1;
		*ptr1 = '\0';
		oldch2 = *ptr2;
		*ptr2 = '\0';
		
		/* this cannot happen, as we previously tested to make sure that */
		/* the first string has a non-null segment */
		if( one == ptr1 ) return -1;
		
		/* take care of the case where the two version segments are */
		/* different types: one numeric, the other alpha (i.e. empty) */
		/* numeric segments are always newer than alpha segments */
		/* XXX See patch #60884 (and details) from bugzilla #50977. */
		if( two == ptr2 ) return isnum ? 1 : -1;

		if( isnum ){
			/* this used to be done by converting the digit segments */
			/* to ints using atoi() - it's changed because long  */
			/* digit segments can overflow an int - this should fix that. */
			
			/* throw away any leading zeros - it's a number, right? */
			while( *one == '0' ) one++;
			while( *two == '0' ) two++;
			
			/* whichever number has more digits wins */
			if( strlen(one) > strlen(two) ) return 1;
			if( strlen(two) > strlen(one) ) return -1;
		}

		/* strcmp will return which one is greater - even if the two */
		/* segments are alpha or if they are numeric.  don't return  */
		/* if they are equal because there might be more segments to */
		/* compare */
		rc = strcmp(one, two);
		if( rc ) return rc < 1 ? -1 : 1;
		/* restore character that was replaced by null above */
		*ptr1 = oldch1;
		one = ptr1;
		*ptr2 = oldch2;
		two = ptr2;
	}
	/* this catches the case where all numeric and alpha segments have */
	/* compared identically but the segment separating characters were */
	/* different */
	if( (!*one) && (!*two) ) return 0;
	/* the final showdown. we never want a remaining alpha string to
	 * beat an empty string. the logic is a bit weird, but:
	 * - if one is empty and two is not an alpha, two is newer.
	 * - if one is an alpha, two is newer.
	 * - otherwise one is newer.
	 * */
	return ( (!*one && !isalpha((int)*two)) || isalpha((int)*one) ) ? -1 : 1;
}

// 0 eq
// 1 a > b
//-1 a < b
int pkg_vercmp(const char *a, const char *b){
	char full1[512];
	char full2[512];
	const char *epoch1, *ver1, *rel1;
	const char *epoch2, *ver2, *rel2;
	int ret;

	/* ensure our strings are not null */
	if     ( !a ) return !b ? 0 : -1;
	else if( !b ) return 1;
	
	const unsigned la = strlen(a);
	const unsigned lb = strlen(b);
	/* another quick shortcut- if full version specs are equal */
	if( la == lb && !strcmp(a, b) ) return 0;
	if( la > 500 || lb > 500 ) die("internal error, version to long");

	/* Parse both versions into [epoch:]version[-release] triplets. We probably
	 * don't need epoch and release to support all the same magic, but it is
	 * easier to just run it all through the same code. */
	memcpy(full1, a, la + 1);
	memcpy(full2, b, lb + 1);
	
	/* parseEVR modifies passed in version, so have to dupe it first */
	parse_evr(full1, &epoch1, &ver1, &rel1);
	parse_evr(full2, &epoch2, &ver2, &rel2);
	
	ret = rpmvercmp(epoch1, epoch2);
	if(ret == 0) {
		ret = rpmvercmp(ver1, ver2);
		if(ret == 0 && rel1 && rel2) {
			ret = rpmvercmp(rel1, rel2);
		}
	}
	return ret;
}

__private pkgdesc_s* generate_db(void* tarbuf){
	tar_s tar;
	tar_mopen(&tar, tarbuf);
	tarent_s* ent;
	pkgdesc_s* db = MANY(pkgdesc_s, 100);
	while( (ent=tar_next(&tar)) ){
		if( ent->type == TAR_FILE ){
			db = mem_upsize(db, 1);
			pkgdesc_parse(&db[mem_header(db)->len++], ent->data, ent->size);
			mem_free(ent);
		}
	}
	if( (errno=tar_errno(&tar)) ){
		mem_free(db);
		return NULL;
	}
	tar_close(&tar);
	return db;
}

int pkgname_cmp(const void* a, const void* b){
	const pkgdesc_s* da = a;
	const pkgdesc_s* db = b;
	return strcmp(da->name, db->name);
}

__private void* get_tar_zst(mirror_s* mirror, const char* repo, const unsigned tos){
	if( mirror->url[0] == '/' ){
		__free char* url = str_printf("%s/%s.db", mirror->url, repo);
		return load_file(url, 1);
	}
	else{
		__free char* url = str_printf("%s/%s/os/%s/%s.db", mirror->url, repo, mirror->arch, repo);
		void* ret = NULL;
		if( !mirror->wwwerror ){
			ret = www_download(url, 0, tos, &mirror->proxy);
		}
		else{
			ret = www_download(url, 0, tos, NULL);
		}
		
		if( mirror->proxy && strcmp(url, mirror->proxy) ) mirror->isproxy = 1;
		
		if( !ret ){
			dbg_error("set error: %u", www_errno());
			mirror->wwwerror = www_errno();
		}
		return ret;
	}
}

//#define IBENCH
__private void mirror_update(mirror_s* mirror, const unsigned tos){
#ifdef IBENCH
	delay_t bench[2][3];
#endif
	const unsigned repocount = sizeof_vector(REPO);
	dbg_info("update %s", mirror->url);
	{
		__free char* rh = www_host_get(mirror->url);
		if( rh ){
			mirror->ping = www_ping(rh);
			//if( mirror->ping < 0 ) dbg_warning("%s fail ping %m", mirror->url);
		}
		else{
			dbg_warning("fail get host: %s", mirror->url);
		}
	}
	
	for( unsigned ir = 0; ir < repocount; ++ir ){
		dbg_info("\t %s", REPO[ir]);
#ifdef IBENCH
		bench[ir][0] = time_cpu_us();
#endif
		__free void* tarzstd = get_tar_zst(mirror, REPO[ir], tos);
		if( !tarzstd ){
			dbg_error("unable to get remote mirror: %s", mirror->url);
			mirror->status = MIRROR_ERR;
			return;
		}
#ifdef IBENCH
		bench[ir][0] = time_cpu_us() - bench[ir][0];
		bench[ir][1] = time_cpu_us();
#endif
		__free void* tarbuf = gzip_decompress(tarzstd);
		if( !tarbuf ){
			dbg_error("decompress zstd archive from mirror: %s", mirror->url);
			mirror->status = MIRROR_ERR;
			switch( errno ){
				case EBADMSG:
					mirror->error  = ERROR_GZIP_DATA;
				break;
				default:
					mirror->error  = ERROR_GZIP;
				break;
			}
			return;
		}
#ifdef IBENCH
		bench[ir][1] = time_cpu_us() - bench[ir][1];
		bench[ir][2] = time_cpu_us();
#endif
		if( !(mirror->repo[ir].db = generate_db(tarbuf)) ){
			switch( errno ){
				case ENOENT : mirror->error = ERROR_TAR_NOBLOCK; break;
				case EBADF  : mirror->error = ERROR_TAR_BLOCKEND; break;  
				case EBADE  : mirror->error = ERROR_TAR_CHECKSUM; break;
				case ENOEXEC: mirror->error = ERROR_TAR_MAGIC; break;
				case EINVAL : mirror->error = ERROR_TAR_KV_ASSIGN; break;
				default: die("internal error, not catch error %d: %s, please report this issue", errno, strerror(errno)); break;
			}
			dbg_error("untar archive from mirror: %s", mirror->url);
			mirror->status = MIRROR_ERR;
			return;
		}
		mem_qsort(mirror->repo[ir].db, pkgname_cmp);
#ifdef IBENCH
		bench[ir][2] = time_cpu_us() - bench[ir][2];
#endif
		mirror->total += mem_header(mirror->repo[ir].db)->len;
		dbg_info("%u package", mirror->total);
	}
#ifdef IBENCH
	for( unsigned i = 0; i < repocount; ++i ){
		dbg_info("bench repo[%u] download: %fs decompress: %fs unpack: %fs", i, bench[i][0]/1000000.0, bench[i][1]/1000000.0, bench[i][2]/1000000.0);
	}
#endif
}
/*
__private void progress_begin(const char* desc){
	fprintf(stderr, "[%5.1f%%] %s", 0.0, desc);
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
*/
void database_local(mirror_s* local, const char* arch){
	mirror_ctor(local, PACMAN_LOCAL_DB, arch, LOCAL_COUNTRY);
	mirror_update(local, 0);
}

char* mirror_loading(const char* fname, const unsigned tos){
	char* buf = fname ? load_file(fname, 1) :  www_download(MIRROR_LIST_URL, 0, tos, NULL);
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

__private int server_unique(mirror_s* mirrors, const char* url){
	mforeach(mirrors, i){
		if( !strcmp(mirrors[i].url, url) ) return 0;
	}
	return 1;
}

mirror_s* mirrors_country(mirror_s* mirrors, const char* mirrorlist, const char* safemirrorlist, const char* country, const char* arch, int uncommented, unsigned type){
	char* url;
	const char* fromcountry = country ? find_country(mirrorlist, country) : mirrorlist;
	
	while( (url=server_url(&fromcountry, uncommented, country ? 1 : 0, type)) ){
		if( server_unique(mirrors, url) ){
			mirrors = mem_upsize(mirrors, 1);
			const unsigned id = mem_header(mirrors)->len++;
			char* fcountry = country ? (char*)country : server_find_country(url, safemirrorlist);
			mirror_ctor(&mirrors[id], url, arch, fcountry);
		}
		else{
			mem_free(url);
		}
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

__private void mirror_cmp_db(mirror_s* local, mirror_s* test){
	const unsigned repocount = sizeof_vector(REPO);
	for( unsigned ir = 0; ir < repocount; ++ir ){
		unsigned const dbcount = mem_header(local->repo[ir].db)->len;
		for( unsigned i = 0; i < dbcount; ++i){
			pkgdesc_s* tpk = mem_bsearch(test->repo[ir].db, &local->repo[ir].db[i], pkgname_cmp);
			if( tpk ){
				int ret = pkg_vercmp(local->repo[ir].db[i].version, tpk->version);
				switch( ret ){
					case -1: ++test->morerecent; break;
					case  1: ++test->outofdate; break;
					case  0: ++test->uptodate; break;
				}
			}
		}
	}
	
	//test->morerecent += test->total - (test->morerecent + test->uptodate + test->outofdate);
}

__private void mirror_store_speed_package(mirror_s* mirror, unsigned speedType){
	static char* SPEED[] = {
		SPEED_LIGHT,
		SPEED_NORMAL,
		SPEED_HEAVY
	};
	mirror->repo[1].speed = MANY(pkgdesc_s, sizeof_vector(SPEED));
	mem_header(mirror->repo[1].speed)->len = speedType;
	for( unsigned i = 0; i < sizeof_vector(SPEED) && i < speedType; ++i ){
		pkgdesc_s  find;
		strcpy(find.name, SPEED[i]);
		pkgdesc_s* pk = mem_bsearch(mirror->repo[1].db, &find, pkgname_cmp);
		if( !pk ){
			die("unable to benchmark mirror, not find %s package", SPEED[i]);
			return;
		}
		memcpy(&mirror->repo[1].speed[i], pk, sizeof(pkgdesc_s));
	}
}

void mirrors_update(mirror_s* local, mirror_s* mirrors, const unsigned ndownload, const unsigned tos, unsigned speedType){
	dbg_info("");
	const unsigned count = mem_header(mirrors)->len;
	const unsigned repocount = sizeof_vector(REPO);
	__atomic unsigned pvalue = 0;
	progress_begin("mirrors updates");
	__paralleft(ndownload)
	for( unsigned i = 0; i < count; ++i){
		mirror_update(&mirrors[i], tos);
		if( mirrors[i].status == MIRROR_UNKNOW ){
			mirror_cmp_db(local, &mirrors[i]);
			mirror_store_speed_package(&mirrors[i], speedType);
			for( unsigned r = 0; r < repocount; ++r ){
				mem_free(mirrors[i].repo[r].db);
			}
		}
		progress_status_refresh(
			mirrors[i].status == MIRROR_ERR ? "fail": "pass",
			mirrors[i].status == MIRROR_ERR ? COLOR_ERR: COLOR_OK,
			mirrors[i].url,
			++pvalue, count
		);
	}
	progress_end();
}

__private int ping_cmp(long a, long b){
	if( a < 0 ){
		if( b < 0 ) return 0;
		return 1;
	}
	if( b < 0 ) return -1;
	return a - b; 
}

__private int status_cmp(mirrorStatus_e a, mirrorStatus_e b){
	if( a == MIRROR_ERR ){
		if( b == MIRROR_ERR ) return 0;
		return 1;
	}
	if( b == MIRROR_ERR ) return -1;
	return 0;
}

__private int sort_real_cmp(const mirror_s* a, const mirror_s* b, const unsigned sort){
	switch( sort ){
		case  0: return strcmp(a->country, b->country);
		case  1: return strcmp(a->url, b->url);
		case  2: return a->isproxy && !b->isproxy ? 1 : !a->isproxy && b->isproxy ? -1 : 0;
		case  3: return status_cmp(a->status, b->status);
		case  4: return a->outofdate - b->outofdate;
		case  5: return b->uptodate - a->uptodate;
		case  6: return b->morerecent - a->morerecent;
		//case  7: return a->retry - b->retry;
		case  7: return a->speed > b->speed ? -1 : a->speed < b->speed ? 1 : 0;
		case  8: return ping_cmp(a->ping, b->ping);
		case  9: return b->estimated - a->estimated;
		default: die("internal error, sort set wrong field");
	}
}

__private int sort_cmp(const void* a, const void* b){
	for( unsigned i = 0; i < SORTCOUNT; ++i ){
		int ret = sort_real_cmp(a, b, SORTMODE[i]);
		if( ret ) return ret;
	}
	return 0;
}

__private unsigned sort_name_to_id(const char* name){
	for( unsigned i = 0; i < sizeof_vector(SORTNAME); ++i ){
		if( !strcmp(SORTNAME[i], name) ){
			return i;
		}
	}
	die("unknow sort mode: %s", name);
}

void mirrors_sort_reset(void){
	SORTCOUNT = 0;
}

void add_sort_mode(const char* mode){
	if( SORTCOUNT >= SORT_MAX ) die("to much sort mode");
	SORTMODE[SORTCOUNT++] = sort_name_to_id(mode);
}

void mirrors_sort(mirror_s* mirrors){
	if( !SORTCOUNT ) die("need to set any or more valid sort modes");
	mem_qsort(mirrors, sort_cmp);
}

__private void mirror_speed(mirror_s* mirror, const char* arch){
	mforeach(mirror->repo[1].speed, it){
		pkgdesc_s* pk = &mirror->repo[1].speed[it];
		__free char* url = str_printf("%s/extra/os/%s/%s", mirror->url, arch, pk->filename);
		double start = time_sec();
		__free void* buf = www_download(url, 0, 0, NULL);
		double stop  = time_sec();
		unsigned size = mem_header(buf)->len;
		mirror->speed += (size / (1024.0*1024.0)) / (stop-start);
	}
	mirror->speed /= (double)mem_header(mirror->repo[1].speed)->len;
}

void mirrors_speed(mirror_s* mirrors, const char* arch){
	const unsigned count = mem_header(mirrors)->len;
	__atomic unsigned pvalue = 0;
	progress_begin("mirrors speed");
	mforeach(mirrors, i){
		if( mirrors[i].status != MIRROR_ERR ){
			mirror_speed(&mirrors[i], arch);
			char tmp[128];
			sprintf(tmp, "%5.1fMiB/s", mirrors[i].speed);
			progress_status_refresh(tmp, COLOR_OK, mirrors[i].url, ++pvalue, count);
		}
		else{
			progress_status_refresh("     error", COLOR_ERR, mirrors[i].url, ++pvalue, count);
		}
	}
	progress_end();
}

__private double std_dev_speed(mirror_s* mirrors, const double avg){
	double sum = 0;
	const unsigned count = mem_header(mirrors)->len;
	for( unsigned i = 0; i < count; ++i ){
		sum += pow(mirrors[i].speed - avg, 2);
	}
	return sqrt(sum / count);
}

/* special thanks Andrea993 for help to adjust this formula */
__private void avg_mirror(mirror_s* mirrors, double* avgSpeed, double* avgOutofdate, double* avgMorerecent){
	*avgSpeed      = 0.0;
	*avgOutofdate  = 0.0;
	*avgMorerecent = 0.0;
	const unsigned count = mem_header(mirrors)->len;
	for( unsigned i = 0; i < count; ++i ){
		*avgSpeed      += mirrors[i].speed;
		*avgOutofdate  += mirrors[i].outofdate;
		*avgMorerecent += mirrors[i].morerecent;
	}
	*avgSpeed      /= count;
	*avgOutofdate  /= count;
	*avgMorerecent /= count;
}

__private double mirror_weight(mirror_s* mirror, const double avgSpeed, const double sddSpeed, const double avgOutofdate, const double avgMorerecent){
	//Gauss
	const double wspeed = (unsigned long)(mirror->speed * 1000000.0) == 0 ? 0.0 : exp(-pow(mirror->speed - avgSpeed, 2) / (2.0 * pow(sddSpeed/WEIGHT_SPEED, 2)));
	//exp dev
	const double lamout = (mirror->outofdate > 2 ? (mirror->outofdate - 2.0) / mirror->outofdate : 1.0) / avgOutofdate;
	const double disout = mirror->outofdate ? exp(-lamout * mirror->outofdate) : 1.0;

	const double lammor = (mirror->morerecent > 2 ? (mirror->morerecent-2.0)/mirror->morerecent : 1.0) / avgMorerecent;
	const double dismor = mirror->morerecent ? exp(-lammor * mirror->morerecent) : 1.0;
	
	const double total = wspeed * disout * dismor;	
	dbg_info("[%2.2f|%2.2f]speed:%5.2f [%2u|%4.1f]outofdate:%5.2f [%2u|%4.1f]morerecent:%5.2f total: %5.2f", 
			mirror->speed, avgSpeed, wspeed, 
			mirror->outofdate, mirror->outofdate * 100.0 / mirror->total, disout, 
			mirror->morerecent, mirror->morerecent * 100.0 / mirror->total, dismor, 
			total
	);
	return total;
}

void mirrors_stability(mirror_s* mirrors){
	double avgSpeed;
	double avgOutofdate;
	double avgMorerecent;	
	avg_mirror(mirrors,&avgSpeed, &avgOutofdate, &avgMorerecent);
	const double sddSpeed = std_dev_speed(mirrors, avgSpeed);
	const unsigned count = mem_header(mirrors)->len;
	for( unsigned i = 0; i < count; ++i ){
		dbg_info("%s", mirrors[i].url);
		mirrors[i].stability = mirror_weight(&mirrors[i], avgSpeed, sddSpeed, avgOutofdate, avgMorerecent);
		mirrors[i].estimated = EXTIMATED_DAY_MIN + (EXTIMATED_DAY_MAX - EXTIMATED_DAY_MIN) * mirrors[i].stability + 0.5;
	}
}


