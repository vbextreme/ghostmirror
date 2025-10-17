#include <notstd/memory.h>
#include <notstd/str.h>
#include <gm/inutility.h>
#include <gm/mirrorlist.h>
#include <gm/www.h>

char* mirrorlist_load(const char* fname){
	char* buf = load_file(fname, 1, 1);
	if( !buf ) die("unable to load mirrorlist from %s", fname);
	return buf;
}

char* mirrorlist_download(const unsigned tos){
	char* buf = www_download(MIRROR_LIST_URL, 0, tos, NULL);
	if( !buf ) die("unable to download mirrorlist from %s", MIRROR_LIST_URL);
	buf = m_nullterm(buf);
	return buf;
}

__private int check_type(const char* url, unsigned type){
	if( (type & MIRROR_TYPE_HTTP ) && !strncmp(url, "http:" , 5) ) return MIRROR_TYPE_HTTP;
	if( (type & MIRROR_TYPE_HTTPS) && !strncmp(url, "https:", 5) ) return MIRROR_TYPE_HTTPS;
	return 0;
}

const char* mirrorlist_find_country(const char* str, const char* country){
	const char* src = str;
	const size_t len = strlen(country);
	while( (str=strstr(str, "## ")) ){
		str += 3;
		if( !strncmp(str, country, len) ){
			str += len;
			while( *str && *str == ' ') ++str;
			if( *str == '\n' ) return str + 1;
		}
	}
	return &src[strlen(src)];
}

const char* mirrorlist_country_next(const char* str){
	while( *str ){
		str = str_skip_h(str);
		if( str[0] == '#' && str[1] == '#' ){
			return str_next_line(str);
		}
		str = str_next_line(str);
	}
	return str;
}

char* mirrorlist_server_next(const char** pline, int uncommented, int breakcountry, unsigned type){
	const char* line = *pline;	
	while( *line ){
		line = str_skip_hn(line);
		if( !*line ) return NULL;
		if( breakcountry && line[0] == '#' && line[1] == '#' ) break;
		if( *line == '#' && uncommented ){
			dbg_info("line is commented but required only uncommented, can skip");
			line = str_next_line(line);
			continue;
		}
		char comment = *line;
		if( *line == '#' ) ++line;
		
		line = str_skip_h(line);
		if( strncmp(line, "Server", 6) ){
			if( comment == '#' ){
				dbg_info("line is commented can skip is not server");
				line = str_next_line(line);
				continue;
			}
			die("malformed mirrorlist, aspected 'Server = <URL>'");
		}
		line = str_skip_h(line+6);
		if( *line != '=' ){
			if( comment == '#' ){
				dbg_info("line is commented can skip is not server");
				line = str_next_line(line);
				continue;
			}
			die("malformed mirrorlist, aspected 'Server = <URL>'");
		}
		line = str_skip_h(line+1);
		const char* url = line;
		const char* endurl = strpbrk(url, "$\n");
		if( !endurl ){
			if( comment == '#' ){
				dbg_info("line is commented can skip is not server");
				line = str_next_line(line);
				continue;
			}
			die("malformed mirrorlist, aspected 'Server = <URL>'");
		}
		if( *endurl == '\n' ){
			if( comment == '#' ){
				dbg_info("line is commented can skip is not server");
				line = endurl+1;
				continue;
			}
			die("malformed mirrorlist, aspected 'Server = <URL>' and url end with $");
		}
		*pline = str_next_line(endurl);
		--endurl;
		if( !check_type(url, type) ){
			dbg_info("not accepted this url type, skip");
			line = str_next_line(endurl);
			continue;
		}
		dbg_info("find url: %.*s", (int)(endurl-url), url);
		return str_dup(url, endurl - url);
	}
	return NULL;
}

char* mirrorlist_country_dup(const char* p){
	if( *p == '#' ) ++p;
	if( *p == '#' ) ++p;
	p = str_skip_h(p);
	const char* country = p;
	p = strchrnul(p, '\n');
	while( p[-1] != '#' && p[-1] == ' ' ) --p;
	return str_dup(country, p-country);
}

__private const char* tocountry(const char* p, const char* mirrorlist){
	while( p > mirrorlist ){
		if( p[0] == '#' && p[-1] == '#' ){
			return &p[-1];
		}
		--p;
	}
	return NULL;
}

const char* mirrorlist_find_country_byurl(const char* mirrorlist, const char* url){
	const char* p;
	if( !(p=strstr(mirrorlist, url)) ) return NULL;
	return tocountry(p, mirrorlist);
}


















