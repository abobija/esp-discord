#include "discord/utils.h"
#include "esp_timer.h"
#include "stdarg.h"
#include "string.h"
#include "esp_heap_caps.h"

uint64_t discord_tick_ms() {
    return esp_timer_get_time() / 1000;
}

char* _discord_strcat(const char* str, ...) {
    const char* first = str;
    size_t length = 0;
    va_list count;
    va_list copy;

    va_start(count, str);
    va_copy(copy, count);
    while(str) {
        length += strlen(str);
        str = va_arg(count, const char*);
    }
    va_end(count);

    if(length <= 0) {
        va_end(copy);
        return NULL;
    }
    
    char* res = malloc(length + 1);

    if(!res) {
        va_end(copy);
        return NULL;
    }

    size_t offset = 0;
    str = first;

    while(str) {
        size_t _len = strlen(str);

        if(_len > 0) {
            memcpy(res + offset, str, _len);
            offset += _len;
        }

        str = va_arg(copy, const char*);
    }
    va_end(copy);
    
    res[length] = '\0';

    return res;
}