#ifndef __NOTSTD_CORE_REGEX_H__
#define __NOTSTD_CORE_REGEX_H__

#include <notstd/core.h>
#include <notstd/utf8.h>
#include <notstd/dict.h>
#include <notstd/regexerr.h>

typedef struct regex regex_t;

void re_test(void);

/*
regex_t* regexu(const utf8_t* regstr);
regex_t* regex(const char* regstr);
const char* regex_error(regex_t* rex);
void regex_error_show(regex_t* rex);
const utf8_t* regex_get(regex_t* rx);

dict_t* match_at(regex_t* rx, const utf8_t* begin, const utf8_t** str);
dict_t* matchuf(regex_t* rx, const utf8_t* begin, const utf8_t** str);
dict_t* matchf(regex_t* rx, const char* begin, const char** str);
*/

#endif
