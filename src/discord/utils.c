#include "discord/utils.h"
#include "esp_timer.h"
#include "stdarg.h"
#include "string.h"
#include "esp_heap_caps.h"
#include "ctype.h"

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

bool discord_strsw(const char* str1, const char* str2) {
    if(!str1 || !str2) {
        return false;
    }

    size_t i = 0;
    for(; str2[i] && str1[i] && str2[i] == str1[i]; i++);
    
    return i > 0 && !str2[i];
}

bool discord_strew(const char* str1, const char* str2) {
	if(!str1 || !str2)
		return false;
	
	size_t len1 = strlen(str1);

	if(len1 == 0)
		return false;

	int diff = len1 - strlen(str2);
    
	if(diff < 0 || diff == len1) // str2 is bigger or empty
		return false;
	
	int i = len1 - 1;
	for(; (i - diff) >= 0 && str1[i] == str2[i - diff]; i--);

	return i < diff;
}

bool discord_streq(const char* str1, const char* str2) {
    return strcmp(str1, str2) == 0;
}

char* discord_url_encode(const char* str) {
    if(!str) { return NULL; }
    static char hex[] = "0123456789abcdef";
    size_t _len = strlen(str);
    char* buf = malloc(_len * 3 + 1); // optimize?
    if(!buf) { return NULL; }
    char* pbuf = buf;

    for(size_t i = 0; i < _len; i++) {
        if (isalnum(str[i]) || strchr(".-_~", str[i])) {
            *pbuf++ = str[i];
        } else if (str[i] == ' ') {
            *pbuf++ = '+';
        } else {
            *pbuf++ = '%';
            *pbuf++ = hex[(str[i] >> 4) & 15];
            *pbuf++ = hex[str[i] & 15];
        }
    }

    *pbuf = '\0';

    return buf;
}