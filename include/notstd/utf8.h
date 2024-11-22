#ifndef __NOTSTD_CORE_UTF8_H__
#define __NOTSTD_CORE_UTF8_H__

#include <notstd/core.h>

#ifndef UTF8_IMPLEMENTS
extern const unsigned UTF8_NB_MAP[256];
#endif

#define U8_CH_MAX 5

#define U8(S) (const utf8_t*)(S)

#define UNICODE_PLANE0_BASIC_MULTILINGUAL_S              0x0000
#define UNICODE_PLANE0_BASIC_MULTILINGUAL_E              0xFFFF
#define UNICODE_PLANE0_PRIVATE_USED_AREA_S               0xE000
#define UNICODE_PLANE0_PRIVATE_USED_AREA_E               0xF8FF
#define UNICODE_PLANE1_SUPPLEMENTARY_MULTILINGUAL_S      0x10000
#define UNICODE_PLANE1_SUPPLEMENTARY_MULTILINGUAL_E      0x1FFFF
#define UNICODE_PLANE2_SUPPLEMENTARY_IDEOGRAPHICS_S      0x20000
#define UNICODE_PLANE2_SUPPLEMENTARY_IDEOGRAPHI CS_E     0x2FFFF
#define UNICODE_PLANE3_UNASIGNED_S                       0x30000
#define UNICODE_PLANE3_UNASIGNED_E                       0x3FFFF
#define UNICODE_PLANE4_UNASIGNED_S                       0x40000
#define UNICODE_PLANE4_UNASIGNED_E                       0x4FFFF
#define UNICODE_PLANE5_UNASIGNED_S                       0x50000
#define UNICODE_PLANE5_UNASIGNED_E                       0x5FFFF
#define UNICODE_PLANE6_UNASIGNED_S                       0x60000
#define UNICODE_PLANE6_UNASIGNED_E                       0x6FFFF
#define UNICODE_PLANE7_UNASIGNED_S                       0x70000
#define UNICODE_PLANE7_UNASIGNED_E                       0x7FFFF
#define UNICODE_PLANE8_UNASIGNED_S                       0x80000
#define UNICODE_PLANE8_UNASIGNED_E                       0x8FFFF
#define UNICODE_PLANE9_UNASIGNED_S                       0x90000
#define UNICODE_PLANE9_UNASIGNED_E                       0x9FFFF
#define UNICODE_PLANE10_UNASIGNED_S                      0xA0000
#define UNICODE_PLANE10_UNASIGNED_E                      0xAFFFF
#define UNICODE_PLANE11_UNASIGNED_S                      0xB0000
#define UNICODE_PLANE11_UNASIGNED_E                      0xBFFFF
#define UNICODE_PLANE12_UNASIGNED_S                      0xC0000
#define UNICODE_PLANE12_UNASIGNED_E                      0xCFFFF
#define UNICODE_PLANE13_UNASIGNED_S                      0xD0000
#define UNICODE_PLANE13_UNASIGNED_E                      0xDFFFF
#define UNICODE_PLANE14_SUPPLEMENTARY_SPECIAL_PURPOSE_S  0xE0000
#define UNICODE_PLANE14_SUPPLEMENTARY_SPECIAL_PURPOSE_E  0xEFFFF
#define UNICODE_PLANE15_SUPPLEMENTARY_PRIVATE_USE_AREA_S 0xF0000
#define UNICODE_PLANE15_SUPPLEMENTARY_PRIVATE_USE_AREA_E 0xFFFFF
#define UNICODE_PLANE16_SUPPLEMENTARY_PRIVATE_USE_AREA_S 0x100000
#define UNICODE_PLANE16_SUPPLEMENTARY_PRIVATE_USE_AREA_E 0x10FFFF

#define NOSTD_UNICODE_PROPERTY_S UNICODE_PLANE15_SUPPLEMENTARY_PRIVATE_USE_AREA_S
#define NOSTD_UNICODE_PROPERTY_E UNICODE_PLANE15_SUPPLEMENTARY_PRIVATE_USE_AREA_E

typedef unsigned char utf8_t;
typedef unsigned int ucs4_t;

//https://github.com/JuliaStrings/utf8proc/blob/master/utf8proc.h
typedef enum {
	UTF8_GB_NULL = 0,
	UTF8_GB_PREPEND,
	UTF8_GB_CR,
	UTF8_GB_LF,
	UTF8_GB_CONTROL,
	UTF8_GB_EXTENDED,
	UTF8_GB_REGIONAL_INDICATOR,
	UTF8_GB_SPACING_MARK,
	UTF8_GB_L,
	UTF8_GB_V,
	UTF8_GB_T,
	UTF8_GB_LV,
	UTF8_GB_LVT,
	UTF8_GB_EMOGI_BASE,
	UTF8_GB_EMOGI_MODIFIERS,
	UTF8_GB_ZWJ,
	UTF8_GB_GLUE_AFTER_ZWJ,
	UTF8_GB_EMOGI_BASE_GAZ,
	UTF8_GB_OTHER
}utf8gbProperty_e;

UNSAFE_BEGIN("-Wunused-function");

__private unsigned utf8_codepoint_nb(utf8_t u){
	return UTF8_NB_MAP[u];
}

UNSAFE_END;

size_t utf8_bytes_count(const utf8_t* u);
const utf8_t* utf8_codepoint_next(const utf8_t* u);
const utf8_t* utf8_codepoint_prev(const utf8_t* u, const utf8_t* start);
size_t utf8_codepoint_count(const utf8_t* u);
const utf8_t* utf8_grapheme_next(const utf8_t* u);
const utf8_t* utf8_grapheme_prev(const utf8_t* u, const utf8_t* start);
size_t utf8_grapheme_count(const utf8_t* u);
void utf8_grapheme_get(utf8_t* out, const utf8_t* u);
int utf8_validate(const utf8_t* u);
ucs4_t utf8_to_ucs4(const utf8_t* u);
ucs4_t str_to_ucs4(const char* str);
size_t ucs4_to_utf8(ucs4_t ch, utf8_t* u);
int ucs4_validate(ucs4_t u4);
const utf8_t* utf8_find_ucs4_range(const utf8_t* u, size_t rangeSt, size_t rangeEn);

int utf8_ncmp(const utf8_t* a, const utf8_t* b, unsigned n);
long utf8_tol(const utf8_t* str, const utf8_t** end, unsigned base, int* err);
unsigned long utf8_toul(const utf8_t* str, const utf8_t** end, unsigned base, int* err);

utf8_t* utf8_chcp(utf8_t* dst, const utf8_t* src, unsigned nb);
const utf8_t* utf8_line_end(const utf8_t* u);
utf8_t* utf8_dup(const utf8_t* src, unsigned len);
const utf8_t* utf8_anyof(const utf8_t* str, const utf8_t* any);
const utf8_t* utf8_chr(const utf8_t* str, utf8_t ch);


#endif
