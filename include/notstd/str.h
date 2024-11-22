#ifndef __NOTSTD_CORE_STR_H__
#define __NOTSTD_CORE_STR_H__

#include <notstd/core.h>

#define str_clear(S) do{*(S)=0; }while(0);
#define str_len(S) strlen(S)
#define str_cmp(A,B) strcmp(A,B)
#define str_chr(S,CH) strchrnul(S, CH)

char* str_dup(const char* src, size_t len);
int str_ncmp(const char* a, size_t lenA, const char* b, size_t lenB);
char* str_cpy(char* dst, const char* src);
char* str_vprintf(const char* format, va_list va1, va_list va2);
__printf(1,2) char* str_printf(const char* format, ...);
const char* str_find(const char* str, const char* need);
const char* str_nfind(const char* str, const char* need, size_t max);
const char* str_anyof(const char* str, const char* any);
const char* str_skip_h(const char* str);
const char* str_skip_hn(const char* str);
const char* str_next_line(const char* str);
const char* str_end(const char* str);
void str_swap(char* restrict a, char* restrict b);
void str_chomp(char* str);
void str_toupper(char* dst, const char* str);
void str_tolower(char* dst, const char* str);
void str_tr(char* str, const char* find, const char replace);
const char* str_tok(const char* str, const char* delimiter, int anydel, __out unsigned* len, __out unsigned* next);
char** str_split(const char* str, const char* delimiter, int anydel);
void str_insch(char* dst, const char ch);
void str_ins(char* dst, const char* restrict src, size_t len);
void str_del(char* dst, size_t len);
char* quote_printable_decode(size_t *len, const char* str);
long str_tol(const char* str, const char** end, unsigned base, int* err);
unsigned long str_toul(const char* str, const char** end, unsigned base, int* err);
int chr_escape_decode(const char* str, const char** end);
char* str_escape_decode(const char* str, unsigned len);


char* _itoa (unsigned long long int value, char *buflim, unsigned int base, int upper_case);
unsigned itoa(char* buf, size_t size, unsigned base, int upper, unsigned long long value);



#endif
