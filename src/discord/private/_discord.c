#include "discord/private/_discord.h"
#include "stdarg.h"

uint64_t dc_tick_ms(void) {
    return esp_timer_get_time() / 1000;
}

char* _dc_strcat(const char* str, ...) {
    va_list arg;
    va_start(arg, str);

    char* res = calloc(0, sizeof(char));
    int start_pos = 0;
    int len = 0;

    while(str) {
        int _len = strlen(str);
        len += _len;
        char* _res = realloc(res, len * sizeof(char));
        if(_res) {
            res = _res;
            _res = NULL;
        } else {
            free(res);
            res = NULL;
            break;
        }
        memcpy(res + start_pos, str, _len);
        start_pos += _len;
        str = va_arg(arg, const char*);
    }

    va_end(arg);

    char* _res = realloc(res, (len + 1) * sizeof(char));
    
    if(_res) {
        res = _res;
        _res = NULL;
        res[len] = '\0';
    } else {
        free(res);
        res = NULL;
    }

    return res;
}