#include "discord/utils.h"
#include "esp_timer.h"
#include "stdarg.h"
#include "string.h"

uint64_t discord_tick_ms() {
    return esp_timer_get_time() / 1000;
}

char* _discord_strcat(const char* str, ...) {
    va_list arg;
    char* res = NULL;
    int start_pos = 0;
    int len = 0;

    va_start(arg, str);

    while(str) {
        int _len = strlen(str);

        if(_len > 0) {
            char* _res = realloc(res, (len += _len));
            if(_res) {
                res = _res;
            } else {
                free(res);
                res = NULL;
                break;
            }
            memcpy(res + start_pos, str, _len);
            start_pos += _len;
        }

        str = va_arg(arg, const char*);
    }

    va_end(arg);

    char* _res = realloc(res, len + 1);
    
    if(_res) {
        res = _res;
        res[len] = '\0';
    } else {
        free(res);
        res = NULL;
    }

    return res;
}