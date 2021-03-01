#ifndef _DISCORD_UTILS_H_
#define _DISCORD_UTILS_H_

#include "stdint.h"
#include "stdbool.h"
#include "stddef.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Universal struct (type) constructor. Resulting object needs to be freed with corresponding object *_free function
 * @param type Struct type name (ex: discord_person_t)
 * @param ... Struct attributes (ex: .id = 2, .name = strdup("John"))
 * @return Pointer to dynamically allocated struct
 */
#define discord_ctor(type, ...) ({ type* obj = calloc(1, sizeof(type)); if(obj) { *obj = (type){ __VA_ARGS__ }; } obj; })

/**
 * @brief Free the list (array of pointers)
 * @param list Double pointer to list
 * @param len Number of the items in the list
 * @param free_fnc Function for freeing the list item
 * @return void
 */ 
#define discord_list_free(list, len, free_fnc) if(list) { for(size_t i = 0; i < len; i++) { free_fnc(list[i]); list[i] = NULL; } free(list); }

/**
 * @brief Concatenate optional number of strings. User need to release the resulting string with a free function
 * @return Pointer to result string or NULL on failure (no memory)
 */
#define discord_strcat(...) _discord_strcat(__VA_ARGS__, NULL)

/**
 * @brief Get time in miliseconds since boot
 * @return number of miliseconds since esp_timer_init was called (this normally happens early during application startup)
 */
uint64_t discord_tick_ms();

/**
 * @brief Don't use this function. Use discord_strcat macro instead.
 */
char* _discord_strcat(const char* str, ...);

/**
 * @brief Check if one string starts with another
 * @param str1 First string
 * @param str2 Second string
 * @return true if first string starts with second one.
 *         In special cases when second or both strings are empty, function will return false
 */
bool discord_strsw(const char* str1, const char* str2);

/**
 * @brief Check if one string ends with another
 * @param str1 First string
 * @param str2 Second string
 * @return true if first string ends with second one.
 *         In special cases when second or both strings are empty, function will return false
 */
bool discord_strew(const char* str1, const char* str2);

/**
 * @brief Check if two strings are equal
 * @param str1 First string
 * @param str2 Second string
 * @return true if first and second strings are equal
 */
bool discord_streq(const char* str1, const char* str2);

/**
 * @brief Http url string encoding.
 *        Resulting encoded string of this functions needs to be freed with free function
 * @param str String which needs to be encoded
 * @return Pointer to encoded string
 */
char* discord_url_encode(const char* str);

#ifdef __cplusplus
}
#endif

#endif