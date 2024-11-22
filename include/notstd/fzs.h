#ifndef __NOTSTD_FZS_H__
#define __NOTSTD_FZS_H__

#include <notstd/core.h>

/**structure to use in vector for fzs qsort*/
typedef struct fzs{
	const char* str; /**< pointer to string*/
	size_t len;      /**< len of string*/
	size_t distance; /**< laste distance calcolate*/
}fzs_s;

typedef size_t(*fzs_f)(const char *a, const size_t lena, const char *b, const size_t lenb);

/** calcolate levenshtein 
 * @param a 
 * @param lena if 0 calcolate strlen a
 * @param b
 * @param lenb if 0 calcolate strlen b
 * @return a size_t, depicting the difference between `a` and `b`, See <https://en.wikipedia.org/wiki/Levenshtein_distance> for more information.
 */
size_t fzs_levenshtein(const char *a, const size_t lena, const char *b, const size_t lenb);
size_t fzs_case_levenshtein(const char *a, size_t lena, const char *b, size_t lenb);
size_t fzs_damerau_levenshtein(const char *s, size_t lena, const char* t, size_t lenb);
size_t fzs_case_damerau_levenshtein(const char *s, size_t lena, const char* t, size_t lenb);
size_t fzs_case_weigth_levenshtein(const char *a, size_t lena, const char* b, size_t lenb);

/** find a element with minimal distance
 * @param v char** 
 * @param count numbers od char*
 * @param str string to search
 * @param lens len of string, if 0 auto strlen
 */
ssize_t fzs_vector_find(char** v, unsigned count, const char* str, unsigned lens, fzs_f fn);

/** reorder vector of fzse to distance of str
 * @param fzse array of fzsElement_s
 * @param count numbers of fzse elements;
 * @param str string to reordering distance
 * @param lens len of string, if 0 auto strlen
 */
void fzs_qsort(fzs_s* fzse, unsigned count, const char* str, unsigned lens, fzs_f fn);

#endif
