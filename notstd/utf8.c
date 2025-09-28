#include <notstd/str.h>
#include "utf8_property.h"

const unsigned UTF8_NB_MAP[256] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};

#define UTF8_IMPLEMENTS 1
#include <notstd/utf8.h>

size_t utf8_bytes_count(const utf8_t* u){
	return strlen((char*)u);
}

/* low mem version
 
#define UTF8_CODEPOINT_NB(U) (((0xE5000000 >> (((utf8_t)(U)>>4)<<1)) & 0x03)+1)

__const unsigned utf8_codepoint_nb(utf8_t u){
	return UTF8_CODEPOINT_NB(u);
}

__const unsigned utf8_codepoint_nb(utf8_t u){
	u >>= 4;
	if( u == 0xF ) return 4;
	if( u == 0xE ) return 3;
	if( (u >> 1) == 6 ) return 2;
	return 1;
}
*/

const utf8_t* utf8_codepoint_next(const utf8_t* u){
	return *u ? u + utf8_codepoint_nb(*u) : u;
}

const utf8_t* utf8_codepoint_prev(const utf8_t* u, const utf8_t* start){
	if( u == start ) return u;
	--u;
	while( u > start && (*u>>6) == 2 ){
		--u;
	}
	return u;
}

size_t utf8_codepoint_count(const utf8_t* u){
	size_t count = 0;
	while( *u ){
		u = utf8_codepoint_next(u);
		++count;
	}
	return count;
}

const utf8_t* utf8_grapheme_next(const utf8_t* u){
	//GB1 GB2 
	//interrompi se non c'è testo
	//sot % any
	//any % eot
	if( !*u ) return u;
		
	const utf8_t* tmp;
	ucs4_t c = utf8_to_ucs4(u);
	utf8gbProperty_e p = gb_property(c);
	//dbg_info("ucs4:0x%X '%s'", c, u);
	while( *u && c >= NOSTD_UNICODE_PROPERTY_S && c <= NOSTD_UNICODE_PROPERTY_E ){
		u = utf8_codepoint_next(u);
		c = utf8_to_ucs4(u);
		p = gb_property(c);
		//dbg_info("nostd next:%u", c);
	}

	if( !*u ) return u;

	//GB3 GB4 GB5
	//non rompere crlf fallo prima e dopo i controlli
	//CR x LF
	//control|CR|LF %
	//% control | CR | LF
	if( p == UTF8_GB_CONTROL || p == UTF8_GB_CR ){
		while( p == UTF8_GB_CONTROL ){
			u = utf8_codepoint_next(u);
			c = utf8_to_ucs4(u);
			p = gb_property(c);
		}
		if( p == UTF8_GB_CR ){
			u = utf8_codepoint_next(u);
			c = utf8_to_ucs4(u);
			p = gb_property(c);
			if( p == UTF8_GB_LF ){
				//dbg_info("gb3/4/5 gb_lf");
				return utf8_codepoint_next(u);
			}
		}
		//dbg_info("gb3/4/5");
		return u;
	}

	//GB6 GB7 GB8
	//non interrompere sillabe Hangul
	//L x (L|V|LV|LVT)
	//(LV|V) x (V|T)
	//(LV|T) x T
	if( p == UTF8_GB_L ){
		do{
			u = utf8_codepoint_next(u);
			c = utf8_to_ucs4(u);
			p = gb_property(c);
		}while( p == UTF8_GB_L || p == UTF8_GB_V || p == UTF8_GB_LV || p == UTF8_GB_LVT );
		//dbg_info("gb6");
		return u;
	}
	if( p == UTF8_GB_LV || p == UTF8_GB_V ){
		do{
			u = utf8_codepoint_next(u);
			c = utf8_to_ucs4(u);
			p = gb_property(c);
		}while( p == UTF8_GB_V || p == UTF8_GB_T );
		//dbg_info("gb7");
		return u;
	}
	if( p == UTF8_GB_T ){
		do{
			u = utf8_codepoint_next(u);
			c = utf8_to_ucs4(u);
			p = gb_property(c);
		}while( p == UTF8_GB_T );
		//dbg_info("gb8");
		return u;
	}

	//GB9a GB9b
	//non interrompere prima di spacing marks o dopo i caratteri prepend
	// x spacing marks
	// prepend x
	tmp = utf8_codepoint_next(u);
	if( gb_property(utf8_to_ucs4(tmp)) == UTF8_GB_SPACING_MARK ){
		u = tmp;
		while( gb_property(utf8_to_ucs4((u=utf8_codepoint_next(u)))) == UTF8_GB_SPACING_MARK );
		//dbg_info("gb9a");
		return u;
	}
	if( p == UTF8_GB_PREPEND ){
		while( gb_property(utf8_to_ucs4((u=utf8_codepoint_next(u)))) == UTF8_GB_PREPEND );
		//dbg_info("gb9b");
		return u;
	}

	//GB9
	//non interrompere prima di estendere i caratteri o ZWJ (ZWJ lo controllerò dopo
	// x (extend|ZWJ)
	//GB11
	//non iterrompere la sequenza di emoji, o sequenza ZWJ di emoji
	//\p{Extended_Pictographics}Extend*ZWJ x \p{EXTEND_PICTOGRAPHICS}
	tmp = utf8_codepoint_next(u);
	int pn = gb_property(utf8_to_ucs4(tmp));
	if( pn == UTF8_GB_EXTENDED || pn == UTF8_GB_ZWJ){
		//dbg_info("next extend || jwj");
		int rtnext;
		do{
			rtnext = 1;
			u = utf8_codepoint_next(u);
			p = gb_property(utf8_to_ucs4(u));
			//dbg_info("next p:%u", p);
			while( p == UTF8_GB_EXTENDED ){
				//dbg_info("skip all extend");
				u = utf8_codepoint_next(u);
				p = gb_property(utf8_to_ucs4(u));
				rtnext = 0;
			}
		}while( p == UTF8_GB_ZWJ );
		//dbg_info("gb9/11");
		return rtnext ? utf8_codepoint_next(u) : u;
	}

	//GB12 GB13
	//non iterrompere la sequenza di emoji, non interrompere simboli regionali RI se è presente un numero di caratteri dispari prima del punto di interruzione
	// sot (RI RI)*RI x RI
	// [^RI](RI RI)*RI x RI
	if( p == UTF8_GB_REGIONAL_INDICATOR ){
		do{
			u = utf8_codepoint_next(u);
			p = gb_property(utf8_to_ucs4(u));
		}while( p == UTF8_GB_REGIONAL_INDICATOR );
		//dbg_info("gb12");
		return u;
	}
	if( p == UTF8_GB_EMOGI_BASE || p == UTF8_GB_EMOGI_BASE_GAZ ){
		do{
			u = utf8_codepoint_next(u);
			p = gb_property(utf8_to_ucs4(u));
		}while( p == UTF8_GB_EMOGI_MODIFIERS );
		//dbg_info("gb13");
		return u;
	}

	//GB999
	//altrimenti rompi ovunque
	// any % any
	u = utf8_codepoint_next(u);
	//dbg_info("gb999");
	return u;		
}

__private const utf8_t* ugp_ctrl(const utf8_t* u, const utf8_t* start){
	if( u == start ) return u;
	const utf8_t* p = utf8_codepoint_prev(u,start);
	ucs4_t c = utf8_to_ucs4(p);
	if( c < NOSTD_UNICODE_PROPERTY_S || c > NOSTD_UNICODE_PROPERTY_E ) return u;

	while( c >= NOSTD_UNICODE_PROPERTY_S && c <= NOSTD_UNICODE_PROPERTY_E ){
		if( p == start ) return start;
		u = p;
		p = utf8_codepoint_prev(p, start);
		c = utf8_to_ucs4(p);
		//dbg_info("nostd prev:0x%X", c);
	}
	return u;
}

const utf8_t* utf8_grapheme_prev(const utf8_t* u, const utf8_t* start){
	if( u == start ) return u;
	u = utf8_codepoint_prev(u, start);
	ucs4_t c = utf8_to_ucs4(u);
	//dbg_info("prev ucs4:0x%X '%s'", c, u);
	while( u > start && c >= NOSTD_UNICODE_PROPERTY_S && c <= NOSTD_UNICODE_PROPERTY_E ){
		u = utf8_codepoint_prev(u, start);
		c = utf8_to_ucs4(u);
		//dbg_info("nostd, move to prev:0x%X", c);
	}
	if( u == start ) return u;
	//GB1 GB2 
	//interrompi se non c'è testo
	//sot % any
	//any % eot
	//if( !*u ) return u;
	
	//c = utf8_to_ucs4(u);
	utf8gbProperty_e p = gb_property(c);

	//GB3 GB4 GB5
	//non rompere crlf fallo prima e dopo i controlli
	//CR x LF
	//control|CR|LF %
	//% control | CR | LF
	if( p == UTF8_GB_LF ){
		u = utf8_codepoint_prev(u, start);
		c = utf8_to_ucs4(u);
		p = gb_property(c);
		if( p == UTF8_GB_CR ){
			u = utf8_codepoint_prev(u,start);
			c = utf8_to_ucs4(u);
			p = gb_property(c);
		}

		while( p == UTF8_GB_CONTROL ){
			u = utf8_codepoint_prev(u, start);
			c = utf8_to_ucs4(u);
			p = gb_property(c);
		}
		//dbg_info("gb3/4/5"); 
		return ugp_ctrl(utf8_codepoint_next(u), start);
	}

	//GB6 GB7 GB8
	//non interrompere sillabe Hangul
	//L x (L|V|LV|LVT)
	//(LV|V) x (V|T)
	//(LV|T) x T
	if( p == UTF8_GB_L || p == UTF8_GB_V || p == UTF8_GB_LV || p == UTF8_GB_LVT ){
		do{
			u = utf8_codepoint_prev(u, start);
			c = utf8_to_ucs4(u);
			p = gb_property(c);
		}while( p == UTF8_GB_L || p == UTF8_GB_V || p == UTF8_GB_LV );
		//dbg_info("gb6/7");
		return ugp_ctrl(utf8_codepoint_next(u), start);
	}
	if( p == UTF8_GB_T ){
		do{
			u = utf8_codepoint_prev(u, start);
			c = utf8_to_ucs4(u);
			p = gb_property(c);
		}while( p == UTF8_GB_T || p == UTF8_GB_V || p == UTF8_GB_LV );
		//dbg_info("gb7/8");
		return ugp_ctrl(utf8_codepoint_next(u), start);
	}

	//GB9a GB9b
	//non interrompere prima di spacing marks o dopo i caratteri prepend
	// x spacing marks
	// prepend x
	//tmp = utf8_codepoint_next(u);
	if( p == UTF8_GB_SPACING_MARK ){
		while( gb_property(utf8_to_ucs4((u=utf8_codepoint_prev(u, start)))) == UTF8_GB_SPACING_MARK );
		//dbg_info("gb9a");
		return ugp_ctrl(u, start);
	}
	if( p == UTF8_GB_PREPEND ){
		while( gb_property(utf8_to_ucs4((u=utf8_codepoint_prev(u, start)))) == UTF8_GB_PREPEND );
		//dbg_info("gb9b");
		return ugp_ctrl(u, start);
	}

	//GB9
	//non interrompere prima di estendere i caratteri o ZWJ (ZWJ lo controllerò dopo
	// x (extend|ZWJ)
	//GB11
	//non iterrompere la sequenza di emoji, o sequenza ZWJ di emoji
	//\p{Extended_Pictographics}Extend*ZWJ x \p{EXTEND_PICTOGRAPHICS}
	if( p == UTF8_GB_EXTENDED || p == UTF8_GB_ZWJ){
		do{
			u = utf8_codepoint_prev(u, start);
			p = gb_property(utf8_to_ucs4(u));
			while( p == UTF8_GB_EXTENDED ){
				u = utf8_codepoint_prev(u, start);
				p = gb_property(utf8_to_ucs4(u));
			}
			u = utf8_codepoint_prev(u, start);
			p = gb_property(utf8_to_ucs4(u));	
		}while( p == UTF8_GB_ZWJ );
		//dbg_info("gb9/11");
		return ugp_ctrl(utf8_codepoint_next(u), start);
	}

	//GB12 GB13
	//non iterrompere la sequenza di emoji, non interrompere simboli regionali RI se è presente un numero di caratteri dispari prima del punto di interruzione
	// sot (RI RI)*RI x RI
	// [^RI](RI RI)*RI x RI
	if( p == UTF8_GB_REGIONAL_INDICATOR ){
		do{
			u = utf8_codepoint_prev(u,start);
			p = gb_property(utf8_to_ucs4(u));
		}while( p == UTF8_GB_REGIONAL_INDICATOR );
		//dbg_info("gb12");
		return ugp_ctrl(utf8_codepoint_next(u), start);
	}
	if( p == UTF8_GB_EMOGI_MODIFIERS ){
		do{
			u = utf8_codepoint_prev(u, start);
			p = gb_property(utf8_to_ucs4(u));
		}while( p == UTF8_GB_EMOGI_BASE || p == UTF8_GB_EMOGI_BASE_GAZ);
		//dbg_info("gb13");
		return ugp_ctrl(utf8_codepoint_next(u), start);
	}

	//GB999
	//altrimenti rompi ovunque
	// any % any
	//u = utf8_codepoint_prev(u, start);	
	//dbg_info("gb999");
	return ugp_ctrl(u,start);
}

size_t utf8_grapheme_count(const utf8_t* u){
	size_t count = 0;
	while( *u ){
		u = utf8_grapheme_next(u);
		++count;
	}
	return count;
}

void utf8_grapheme_get(utf8_t* out, const utf8_t* u){
	//dbg_info("");
	const utf8_t* next = utf8_grapheme_next(u);
	*out = 0;
	if( next == u ) return;
	size_t nb = next - u;
	//dbg_info("get nb:%lu", nb);
	while( nb --> 0 ){
		*out++ = *u++;
	}
	*out = 0;
}

//https://stackoverflow.com/questions/1031645/how-to-detect-utf-8-in-plain-c/22135005
int utf8_validate(const utf8_t* u){
	while(*u){
		// ASCII use bytes[0] <= 0x7F to allow ASCII control characters
		if(
			*u == 0x09 ||
            *u == 0x0A ||
            *u == 0x0D ||
            (0x20 <= *u && *u <= 0x7E)
        ){
            ++u;
            continue;
        }

		// non-overlong 2-byte
        if( 
			(0xC2 <= u[0] && u[0] <= 0xDF) &&
            (0x80 <= u[1] && u[1] <= 0xBF)
        ){
            u += 2;
            continue;
        }

	    if( 
			( // excluding overlongs
				u[0] == 0xE0 &&
				(0xA0 <= u[1] && u[1] <= 0xBF) &&
                (0x80 <= u[2] && u[2] <= 0xBF)
            ) ||
            (// straight 3-byte
                ((0xE1 <= u[0] && u[0] <= 0xEC) || u[0] == 0xEE || u[0] == 0xEF) &&
                (0x80 <= u[1] && u[1] <= 0xBF) &&
                (0x80 <= u[2] && u[2] <= 0xBF)
            ) ||
            (// excluding surrogates
                u[0] == 0xED &&
                (0x80 <= u[1] && u[1] <= 0x9F) &&
                (0x80 <= u[2] && u[2] <= 0xBF)
            )
        ) {
            u += 3;
            continue;
        }

        if( 
			(// planes 1-3
                u[0] == 0xF0 &&
                (0x90 <= u[1] && u[1] <= 0xBF) &&
                (0x80 <= u[2] && u[2] <= 0xBF) &&
                (0x80 <= u[3] && u[3] <= 0xBF)
            ) ||
            (// planes 4-15
                (0xF1 <= u[0] && u[0] <= 0xF3) &&
                (0x80 <= u[1] && u[1] <= 0xBF) &&
                (0x80 <= u[2] && u[2] <= 0xBF) &&
                (0x80 <= u[3] && u[3] <= 0xBF)
            ) ||
            (// plane 16
                u[0] == 0xF4 &&
                (0x80 <= u[1] && u[1] <= 0x8F) &&
                (0x80 <= u[2] && u[2] <= 0xBF) &&
                (0x80 <= u[3] && u[3] <= 0xBF)
            )
        ){
            u += 4;
            continue;
        }
        return 0;
    }
    return 1;
}

//krb5/src/util/support/utf8.c.html
ucs4_t utf8_to_ucs4(const utf8_t* u){
    static unsigned char mask[] = { 0, 0x7f, 0x1f, 0x0f, 0x07 };
	size_t len = utf8_codepoint_nb(*u);
	//dbg_info("len:%lu", len);
    ucs4_t ch = u[0] & mask[len];
    for( size_t i = 1; i < len; ++i ){
		if( (u[i] & 0xc0) != 0x80) return (ucs4_t)-1;
        ch <<= 6;
        ch |= u[i] & 0x3f;
    }
    return ch > 0x10ffff ? (ucs4_t)-1 : ch;
}

ucs4_t str_to_ucs4(const char* str){
	return utf8_to_ucs4((const utf8_t*)str);
}

//krb5/src/util/support/utf8.c.html
size_t ucs4_to_utf8(ucs4_t ch, utf8_t* u){
    size_t len = 0;
    if( ch > 0x10ffff) return 0;
    if( u == NULL ){
        if (ch < 0x80) return 1;
        else if (ch < 0x800) return 2;
        else if (ch < 0x10000) return 3;
        else return 4;
    }

    if( ch < 0x80) {
        u[len++] = ch;
    }else if( ch < 0x800 ){
        u[len++] = 0xc0 | ( ch >> 6 );
        u[len++] = 0x80 | ( ch & 0x3f );
    }else if( ch < 0x10000 ){
        u[len++] = 0xe0 | ( ch >> 12 );
        u[len++] = 0x80 | ( (ch >> 6) & 0x3f );
        u[len++] = 0x80 | ( ch & 0x3f );
    }else{
        u[len++] = 0xf0 | ( ch >> 18 );
        u[len++] = 0x80 | ( (ch >> 12) & 0x3f );
        u[len++] = 0x80 | ( (ch >> 6) & 0x3f );
        u[len++] = 0x80 | ( ch & 0x3f );
    }
	u[len] = 0;
    return len;
}

const utf8_t* utf8_find_ucs4_range(const utf8_t* u, size_t rangeSt, size_t rangeEn){
	while( *u ){
		if( *u & 0x80 ){
			ucs4_t u4 = utf8_to_ucs4(u);
			if( u4 >= rangeSt && u4 <= rangeEn ) return u;
			u += utf8_codepoint_nb(*u);
		}
		else{
			++u;
		}
	}

	return u;
}

int ucs4_validate(ucs4_t u4){
	utf8_t u8[5];
	ucs4_to_utf8(u4, u8);
	return utf8_validate(u8);
}

int utf8_ncmp(const utf8_t* a, const utf8_t* b, unsigned n){
	return memcmp(a, b, n);
}

long utf8_tol(const utf8_t* str, const utf8_t** end, unsigned base, int* err){
	const char* s = (const char*)str;
	char* e;
	errno = 0;
	long ret = strtol(s, &e, base);
	if( errno || !e || e == s ){
		if( err ) *err = 1;
		if( !errno ) errno = EINVAL;
		if( end )*end = str;
		return 0;
	}
	if( err ) *err = 0;
	if( end ) *end = (const utf8_t*)e;
	return ret;
}

unsigned long utf8_toul(const utf8_t* str, const utf8_t** end, unsigned base, int* err){
	const char* s = (const char*)str;
	char* e;
	errno = 0;
	unsigned long ret = strtoul(s, &e, base);
	if( errno || !e || e == s ){
		if( err ) *err = 1;
		if( !errno ) errno = EINVAL;
		if( end ) *end = str;
		return 0;
	}
	if( err ) *err = 0;
	if( end ) *end = (const utf8_t*)e;
	return ret;
}

utf8_t* utf8_chcp(utf8_t* dst, const utf8_t* src, unsigned nb){
	if( !nb ) nb = utf8_codepoint_nb(*src);
	for( unsigned i = 0; i < nb; ++i ){
		*dst++ = *src++;
	}
	return dst;
}

const utf8_t* utf8_line_end(const utf8_t* u){
	return U8(strchrnul((const char*)u, '\n'));
}

utf8_t* utf8_dup(const utf8_t* src, unsigned len){
	return (utf8_t*)str_dup((const char*)src, len);
}

const utf8_t* utf8_anyof(const utf8_t* str, const utf8_t* any){
	size_t ret = strcspn((const char*)str, (const char*)any); 
	return str+ret;
}

const utf8_t* utf8_chr(const utf8_t* str, utf8_t ch){
	return U8(str_chr((const char*)str, ch));
}


