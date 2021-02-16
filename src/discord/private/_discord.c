#include "discord/private/_discord.h"

uint64_t dc_tick_ms(void) {
    return esp_timer_get_time() / 1000;
}

char* dc_strcat(const char* str1, const char* str2) {
    int len1 = strlen(str1);
    int len2 = strlen(str2);
    char* res = malloc(len1 + len2 + 1);
    
    memcpy(res, str1, len1);
    memcpy(res + len1, str2, len2);
    res[len1 + len2] = '\0';

    return res;
}