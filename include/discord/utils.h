#ifndef _DISCORD_UTILS_H_
#define _DISCORD_UTILS_H_

#include "stdint.h"
#include "stdbool.h"
#include "stddef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define discord_list_free(list, len, free_fnc) if(list) { for(size_t i = 0; i < len; i++) { free_fnc(list[i]); list[i] = NULL; } free(list); }

/**
 * Concatenate optional number of strings. User need to release the resulting string with a free function
 * @return Pointer to result string or NULL on failure
 */
#define discord_strcat(...) _discord_strcat(__VA_ARGS__, NULL)

/**
 * @brief Get time in miliseconds since boot
 * @return number of miliseconds since esp_timer_init was called (this normally happens early during application startup)
 */
uint64_t discord_tick_ms();

/**
 * Don't use this function. Use discord_strcat macro instead.
 */
char* _discord_strcat(const char* str, ...);

/**
 * @return true if str1 starts with str2
 */
bool discord_strsw(const char* str1, const char* str2);

/**
 * @return true if str1 and str2 are equal
 */
bool discord_streq(const char* str1, const char* str2);

/**
 * @return Pointer to encoded string. Result needs to be freed.
 */
char* discord_url_encode(const char* str);

#ifdef __cplusplus
}
#endif

#endif