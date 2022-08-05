#include "estr.h"
#include "cutils.h"
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

bool estr_eq(const char* str1, const char* str2) {
    if(!str1 || !str2)
        return false;
    
    return strcmp(str1, str2) == 0;
}

bool estrn_eq(const char* str1, const char* str2, size_t n) {
    if(!str1 || !str2)
        return false;
    
    return strncmp(str1, str2, n) == 0;
}

bool estr_sw(const char* str1, const char* str2) {
    if(!str1 || !str2) {
        return false;
    }

    size_t i = 0;
    for(; str2[i] && str1[i] && str2[i] == str1[i]; i++);
    
    return i > 0 && !str2[i];
}

bool estr_sw_chr(const char* str, char chr) {
    return !str ? false : str[0] == chr;
}

bool estr_ew(const char* str1, const char* str2) {
	if(!str1 || !str2)
		return false;
	
	size_t len1 = strlen(str1);

	if(len1 == 0)
		return false;

    size_t len2 = strlen(str2);
    
	if(len2 > len1 || len2 == 0) // str2 is bigger or empty
		return false;
	
    size_t diff = len1 - len2;

    if(diff == 0) {
        return estr_eq(str1, str2);
    }
    
	for(size_t i = len2, j = len1; i > 0; i--, j--) {
        if(str2[i - 1] != str1[j - 1]) {
            return false;
        }
    }

	return true;
}

bool estr_ew_chr(const char* str, char chr) {
    return !str ? false : str[strlen(str) - 1] == chr;
}

bool estrn_is_digit_only(const char* str, size_t n) {
    if(!str)
        return false;

    size_t len = strlen(str);
    size_t limit = n <= len ? n : len;

    for(size_t i = 0; i < limit; i++) {
        if (str[i] < '0' || str[i] > '9')
            return false;
    }

    return true;
}

size_t estrn_chrcnt(const char* str, char chr, size_t n) {
    if(!str) {
        return 0;
    }
    
	size_t cnt = 0;
    size_t len = strlen(str);
    size_t limit = n <= len ? n : len;

	for(size_t i = 0; i < limit; i++) {
		if(str[i] == chr)
			cnt++;
	}

	return cnt;
}

char** estr_split(const char* str, const char chr, size_t* out_len) {
    if(!str || !out_len)
        return NULL;

    char* ptr = (char*) str;
    char prev = '\0';
    size_t len = 0;

    while(*ptr) {
        if(prev && ((*ptr == chr && prev != chr) || (!*(ptr + 1) && *ptr != chr))) len++;
        prev = *ptr;
        ptr++;
    }

    char** result = NULL;

    if((*out_len = len) <= 0) {
        if(!*str || *str == chr)
            return NULL;
        
        result = calloc(*out_len = 1, sizeof(char*));
        if(!result) { *out_len = 0; return NULL; }
        result[0] = strdup(str);
        if(!result[0]) { *out_len = 0; free(result); return NULL; }
        return result;
    }

    result = calloc(len, sizeof(char*));
    if(!result) { *out_len = 0; return NULL; }

    char* start = (char*) str;
    ptr = start;
    prev = '\0';
    size_t i = 0;
    
    while(*ptr) {
        if(*ptr != chr && prev == chr)
            start = ptr;
        
        bool offset_one = !*(ptr + 1) && *ptr != chr;

        if(prev && ((*ptr == chr && prev != chr) || offset_one)) {
            size_t piece_len = (offset_one ? ptr + 1 : ptr) - start;
            result[i] = malloc(piece_len + 1);
            if(!result[i]) {
                size_t _tmp = i + 1;
                cu_list_tfree(result, size_t, _tmp);
                *out_len = 0;
                return NULL;
            }
            memcpy(result[i], start, piece_len);
            result[i][piece_len] = '\0';
            i++;
        }

        prev = *ptr;
        ptr++;
    }

    return result;
}

char* _estr_cat(const char* str, ...) {
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

char* estr_url_encode(const char* str) {
    if(!str) { return NULL; }
    static char hex[] = "0123456789abcdef";
    size_t _len = strlen(str);
    char* buf = malloc(_len * 3 + 1); // optimize?
    if(!buf) { return NULL; }
    char* pbuf = buf;

    for(size_t i = 0; i < _len; i++) {
        if (estr_is_alnum(str[i]) || strchr(".-_~", str[i])) {
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

char* estr_rep(const char *orig, const char *rep, const char *with) {
    // Taken from: https://stackoverflow.com/a/779960
    // It's a little bit modified

    if (!orig || !rep || !with)
        return NULL;
    
    char *result;  // the return string
    char *ins;     // the next insert point
    char *tmp;     // varies
    int len_rep;   // length of rep (the string to remove)
    int len_with;  // length of with (the string to replace rep with)
    int len_front; // distance between rep and end of last rep
    int count;     // number of replacements
    
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; // empty rep causes infinite loop during count
    
    len_with = strlen(with);

    // count the number of replacements needed
    ins = (char*) orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}

bool estr_is_alnum(char chr) {
    return isalnum(chr);
}

bool estr_chr_is_ws(char chr) {
    return chr == ' '||
        chr == '\t' ||
        chr == '\r' ||
        chr == '\n' ||
        chr == '\v' ||
        chr == '\f';
}

bool estr_is_trimmed(const char* str) {
    return !str ? false :
        (!estr_chr_is_ws(str[0]) && !estr_chr_is_ws(str[strlen(str) - 1]));
}

bool estr_contains_unescaped_chr(const char* str, char chr) {
    if(!str) {
        return false;
    }

    char* curr = (char*) str;
    char* prev = NULL;

    while(*curr) {
        if(*curr == chr && (!prev || *prev != '\\')) {
            return true;
        }

        prev = curr++;
    }

    return false;
}

bool estr_is_empty_ws(const char* str) {
    if(! str) {
        return true;
    }

    char* ptr = (char*) str;

    while(*ptr) {
        if(!estr_chr_is_ws(*ptr++)) {
            return false;
        }
    }

    return true;
}

char* estr_repeat_chr(char chr, unsigned int times) {
    if(times == 0) {
        return NULL;
    }

    char* str = malloc(times + 1);

    if(!str) {
        return NULL;
    }

    for(unsigned int i = 0; i < times; i++) {
        str[i] = chr;
    }

    str[times] = '\0';

    return str;
}

bool estr_contains_ws(const char* str) {
    if(!str) {
        return false;
    }

    char* ptr = (char*) str;

    while(*ptr) {
        if(estr_chr_is_ws(*ptr++)) {
            return true;
        }
    }

    return false;
}

cu_err_t estr_validate(const char* str, estr_validation_t* validation) {
    if(!str || !validation) {
        return CU_ERR_INVALID_ARG;
    }

    if(validation->no_whitespace && estr_contains_ws(str)) {
        return CU_ERR_ESTR_INVALID_WHITESPACE;
    }

    if(validation->length) {
        unsigned int len = strlen(str);

        if(validation->minlen > 0 && validation->maxlen == 0) {
            validation->maxlen = validation->minlen;
        }

        if(len < validation->minlen || len > validation->maxlen) {
            return CU_ERR_ESTR_INVALID_OUT_OF_BOUNDS;
        }
    }

    return CU_OK;
}