#ifndef _CUTILS_ESTR_H_
#define _CUTILS_ESTR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "cutils.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define CU_ERR_ESTR_BASE                    (-1000)
#define CU_ERR_ESTR_INVALID_WHITESPACE      (CU_ERR_ESTR_BASE - 1)
#define CU_ERR_ESTR_INVALID_OUT_OF_BOUNDS   (CU_ERR_ESTR_BASE - 2)

typedef struct {
    bool length;
    unsigned int minlen;
    unsigned int maxlen;
    bool no_whitespace;
} estr_validation_t;

/**
 * @brief Concatenate optional number of strings. User need to release the resulting string with a free function.
 *        Make sure that no one of the strings are NULL, otherwise concatenation will stop on the first NULL.
 * @return Pointer to result string or NULL on failure (no memory)
 */
#define estr_cat(...) _estr_cat(__VA_ARGS__, NULL)

/**
 * @brief Check if two strings are equal
 * @param str1 First string
 * @param str2 Second string
 * @return true if first and second strings are equal
 */
bool estr_eq(const char* str1, const char* str2);

/**
 * @brief Check if two strings are equal
 * @param str1 First string
 * @param str2 Second string
 * @param n Number of character that needs to be tested
 * @return true if first and second strings are equal
 */
bool estrn_eq(const char* str1, const char* str2, size_t n);

/**
 * @brief Check if one string starts with another
 * @param str1 First string
 * @param str2 Second string
 * @return true if first string starts with second one.
 *         In special cases when second or both strings are empty, function will return false
 */
bool estr_sw(const char* str1, const char* str2);

/**
 * @brief Check if string starts with character
 * @param str String
 * @param chr Character
 * @return true if string starts with character
 *         In special case when string is empty, function will return false
 */
bool estr_sw_chr(const char* str, char chr);

/**
 * @brief Check if one string ends with another
 * @param str1 First string
 * @param str2 Second string
 * @return true if first string ends with second one.
 *         In special cases when second or both strings are empty, function will return false
 */
bool estr_ew(const char* str1, const char* str2);

/**
 * @brief Check if string ends with character
 * @param str String
 * @param chr Character
 * @return true if string ends with character
 *         In special case when string is empty, function will return false
 */
bool estr_ew_chr(const char* str, char chr);

/**
 * @brief Check if all characters in string are digits
 * @param str Haystack
 * @param n Number of characters that needs to be tested
 * @return true if all characters are digits
 */
bool estrn_is_digit_only(const char* str, size_t n);

/**
 * @brief Count number of occurences of character in string
 * @param str Haystack
 * @param chr Character that's gonna be counted
 * @param n Number of string characters that needs to be tested
 * @return Number of character occurences
 */
size_t estrn_chrcnt(const char* str, char chr, size_t n);

/**
 * @brief Split string using character. Resulting list needs to be freed (cu_list_free can be used)
 * @param str String that is gonna be used for splitting
 * @param chr Character around which string is be splitted
 * @param out_len Pointer to outer variable in which be stored length of resulting list
 * @return List of strings after splitting
 */
char** estr_split(const char* str, const char chr, size_t* out_len);

/**
 * @brief Don't use this function. Use estr_cat macro instead.
 */
char* _estr_cat(const char* str, ...);

/**
 * @brief Http url string encoding.
 *        Resulting encoded string of this functions needs to be freed with free function
 * @param str String which needs to be encoded
 * @return Pointer to encoded string
 */
char* estr_url_encode(const char* str);

/**
 * @brief Replace string with another string. Result needs to be freed
 * @param orig Original string
 * @param rep Part of the string which needs to be replaced
 * @param with Replacement for rep
 * @return Pointer to result or NULL on failure
 */
char* estr_rep(const char* orig, const char* rep, const char* with);

/**
 * @brief Checks if character is alphanumeric
 * @param chr Character
 * @return true if character is alphanumeric
 */
bool estr_is_alnum(char chr);

/**
 * @brief Check if character is whitespace
 * @param chr Character
 * @return true if character is whitespace
 */
bool estr_chr_is_ws(char chr);

/**
 * @brief Checks if string does not starts or ends with whitespace
 * @param str String that is gonna be tested if it is trimmed
 * @return true if string is trimmed.
 *         If special case when string is null, false will be returned.
 */
bool estr_is_trimmed(const char* str);

/**
 * @brief Checks if string contains unescaped character
 * @param str String
 * @param chr Character
 * @return true if string contains unescaped character.
 *         In special case when string is null, false will be returned
 */
bool estr_contains_unescaped_chr(const char* str, char chr);

/**
 * @brief Checks if string is empty (string is empty if it contains only whitespace chars or his length is zero)
 * @param str String
 * @return true if string is empty (or NULL)
 */
bool estr_is_empty_ws(const char* str);

/**
 * @brief Make string by repeating character multiple times.
 *        Result needs to be freed
 * @param chr Character
 * @param times How much time chr needs to be repeated
 * @return pointer to allocated null-terminated string, or NULL on failure (no memory) or if times == 0
 */
char* estr_repeat_chr(char chr, unsigned int times);

/**
 * @brief Check if string contains whitespace
 * @param str String
 * @return true if string contains whitespace, otherwise false.
 *         If str is NULL, false will be returned.
 */
bool estr_contains_ws(const char* str);

/**
 * @brief Validate string by schema
 * @param str String
 * @param validation Validation schema
 * @return CU_OK if string is valid
 */
cu_err_t estr_validate(const char* str, estr_validation_t* validation);

#ifdef __cplusplus
}
#endif

#endif