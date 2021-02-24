#ifndef _DISCORD_USER_H_
#define _DISCORD_USER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stdbool.h"

typedef struct {
    char* id;
    bool bot;
    char* username;
    char* discriminator;
} discord_user_t;

discord_user_t* discord_user_ctor(char* id, bool bot, char* username, char* discriminator);
discord_user_t* discord_user_clone(discord_user_t* user);
void discord_user_free(discord_user_t* user);

#ifdef __cplusplus
}
#endif

#endif