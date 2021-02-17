#ifndef _DISCORD_MODELS_USER_H_
#define _DISCORD_MODELS_USER_H_

typedef struct {
    char* id;
    bool bot;
    char* username;
    char* discriminator;
} discord_user_t;

#endif