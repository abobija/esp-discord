#ifndef _DISCORD_MODELS_USER_H_
#define _DISCORD_MODELS_USER_H_

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

void discord_model_user_free(discord_user_t* user);

#ifdef __cplusplus
}
#endif

#endif