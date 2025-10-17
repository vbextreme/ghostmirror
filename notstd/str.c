#include <notstd/str.h>
#include <stdarg.h>
#include <ctype.h>

char* str_dup(const char* src, size_t len){
	if( !src ) src = "";
	if( !len ) len=strlen(src);
	char* str = MANY(char, len + 1);
	memcpy(str, src, len);
	str[len] = 0;
	return str;
}

int str_ncmp(const char* a, size_t lenA, const char* b, size_t lenB){
	if( lenA != lenB ) return lenA-lenB;
	return strncmp(a,b, lenA);
}

char* str_cpy(char* dst, const char* src){
	size_t len = strlen(src);
	memcpy(dst, src, len+1);
	return &dst[len];
}

char* str_vprintf(const char* format, va_list va1, va_list va2){
	size_t len = vsnprintf(NULL, 0, format, va1);
	char* ret = MANY(char, len+1);
	vsprintf(ret, format, va2);
	m_header(ret)->len = len;
	return ret;
}

__printf(1,2) char* str_printf(const char* format, ...){
	va_list va1,va2;
	va_start(va1, format);
	va_start(va2, format);
	char* ret = str_vprintf(format, va1, va2);
	va_end(va1);
	va_end(va2);
	return ret;
}

const char* str_find(const char* str, const char* need){
	const char* ret = strstr(str, need);
	return ret ? ret : &str[strlen(str)];
}

const char* str_nfind(const char* str, const char* need, size_t max){
	const char* f = str;
	size_t len = str_len(need);
	size_t lr = max;
	while( (f=memchr(f, *need, lr)) ){
		if( !memcmp(f, need, len) )	return f;
		++f;
		lr = max - (f-str);
	}
	return str+max;	
}

const char* str_anyof(const char* str, const char* any){
	const char* ret = strpbrk(str, any);
	return ret ? ret : &str[strlen(str)];
}

const char* str_skip_h(const char* str){
	while( *str && (*str == ' ' || *str == '\t') ) ++str;
	return str;
}

const char* str_skip_hn(const char* str){
	while( *str && (*str == ' ' || *str == '\t' || *str == '\n') ) ++str;
	return str;
}

const char* str_next_line(const char* str){
	while( *str && *str != '\n' ) ++str;
	if( *str ) ++str;
	return str;
}

const char* str_end(const char* str){
	size_t len = strlen(str);
	return &str[len];
}

void str_swap(char* restrict a, char* restrict b){
	size_t la = strlen(a);
	size_t lb = strlen(b);
	memswap(a, la, b, lb);
}

void str_chomp(char* str){
	const ssize_t len = strlen(str);
	if( len > 0 && str[len-1] == '\n' ){
		str[len-1] = 0;
	}
}

void str_toupper(char* dst, const char* src){
	while( (*dst++=toupper(*src++)) );
}

void str_tolower(char* dst, const char* src){
	while( (*dst++=toupper(*src++)) );
}

void str_tr(char* str, const char* find, const char replace){
	while( (str=strpbrk(str,find)) ) *str++ = replace;
}

const char* str_tok(const char* str, const char* delimiter, int anydel, __out unsigned* len, __out unsigned* next){
	const char* begin = &str[*next];
	if( !*begin ) return begin;
	const char* sn = anydel ? str_anyof(begin, delimiter) : str_find(begin, delimiter);
	*len = sn - begin;
	*next += *sn ? *len + strlen(delimiter) : *len;
	return begin;
}

void str_insch(char* dst, const char ch){
	size_t ld = strlen(dst);
	memmove(dst+1, dst, ld);
	*dst = ch;
}

void str_ins(char* dst, const char* restrict src, size_t len){
	size_t ld = strlen(dst);
	memmove(dst+len, dst, ld+1);
	memcpy(dst, src, len);
}

void str_del(char* dst, size_t len){
	size_t ld = strlen(dst);
	memmove(dst, &dst[len], (ld - len)+1);
}

char* quote_printable_decode(size_t *len, const char* str){
	size_t strsize = strlen(str);
	char* ret = MANY(char, strsize + 1);
	char* next = ret;
	while( *str ){
		if( *str != '=' ){
			*next++ = *str++;
		}
		else{
			++str;
			if( *str == '\r' ) ++str;
			if( *str == '\n' ){
				++str;
				continue;
			}	
			char val[3];
			val[0] = *str++;
			val[1] = *str++;
			val[2] = 0;
			*next++ = strtoul(val, NULL, 16);
		}
	}
	*next = 0;
	if( len ) *len = next - ret;
	return ret;
}

long str_tol(const char* str, const char** end, unsigned base, int* err){
	char* e;
	errno = 0;
	long ret = strtol(str, &e, base);
	if( errno || !e || str == e){
		if( err ) *err = 1;
		if( !errno ) errno = EINVAL;
		if( end ) *end = str;
		return 0;
	}
	if( err ) *err = 0;
	if( end ) *end = e;
	return ret;
}

unsigned long str_toul(const char* str, const char** end, unsigned base, int* err){
	char* e;
	errno = 0;
	unsigned long ret = strtoul(str, &e, base);
	if( errno || !e || str == e ){
		if( err ) *err = 1;
		if( !errno ) errno = EINVAL;
		if( end ) *end = str;
		return 0;
	}
	if( err ) *err = 0;
	if( end ) *end = e;
	return ret;
}

int chr_escape_decode(const char* str, const char** end){
	if( *str != '\\' ){
		if( end ) *end = str+1;
		return *str;
	}
	++str;
	unsigned q = 0;
	int e;
	int ret;

	switch( *str ){
		case 't': if( end ) *end = str+1; return '\t';
		case 'n': if( end ) *end = str+1; return '\n';
		case 'r': if( end ) *end = str+1; return '\r';
		case 'f': if( end ) *end = str+1; return '\f';
		case 'a': if( end ) *end = str+1; return '\a';
		case 'e': if( end ) *end = str+1; return 27;
				  
		case 'x':
			++str;
			if( *str == '{' ){ q = 1; ++str; }
			ret = str_toul(str, end, 16, &e);
			if( e ) return -1;
			if( q ){
				if( *str != '}' ) return -1;
				*end = str+1;
			}
		return ret;

		case 'o':
			++str;
			ret = str_toul(str, end, 8, &e);
			if( e ) return -1;
		return ret;
	
		case '0':
			++str;
			ret = str_toul(str, end, 10, &e);
			if( e ) return -1;
		return ret;

		default: if( end ) *end = str+1; return *str;
	}
}

char* str_escape_decode(const char* str, unsigned len){
	if( !len ) len = strlen(str);
	const char* end = &str[len-1];
	const char* e;
	char* out = MANY(char, len+1);
	char* o = out;
	
	while( str < end ){
		int ch = chr_escape_decode(str, &e);
		if( ch == -1 ) goto ONERR;
		str = e;
		*o++ = ch;
	}
	*o = 0;
	return out;
ONERR:
	m_free(out);
	return NULL;
}













