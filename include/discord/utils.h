#ifndef _DISCORD_UTILS_H_
#define _DISCORD_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

/**
 * Concatenate optional number of strings. User need to release the resulting string with a free function
 * @return Pointer to result string or NULL on failure
 */
#define discord_strcat(...) _discord_strcat(__VA_ARGS__, NULL)

/**
 * @brief Get time in miliseconds since boot
 * @return number of miliseconds since esp_timer_init was called (this normally
 *          happens early during application startup).
 */
uint64_t discord_tick_ms();

/**
 * Don't use this function. Use discord_strcat macro instead.
 */
char* _discord_strcat(const char* str, ...);

#ifdef __cplusplus
}
#endif

#endif