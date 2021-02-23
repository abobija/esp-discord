#ifndef _DISCORD_MODELS_ROLE_H_
#define _DISCORD_MODELS_ROLE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

typedef struct {
    char* id;
    char* name;
    uint8_t position;
    char* permissions;
} discord_role_t;

void discord_role_free(discord_role_t* role);
void discord_role_list_free(discord_role_t** roles, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif