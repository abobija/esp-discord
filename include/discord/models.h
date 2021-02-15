#pragma once

#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char* id;
    bool bot;
    char* username;
    char* discriminator;
} discord_user_t;

typedef struct {
    char* id;
    char* content;
    char* channel_id;
    discord_user_t* author;
} discord_message_t;

discord_user_t* discord_model_user_from_cjson(cJSON* root);
void discord_model_user_free(discord_user_t* user);

discord_message_t* discord_model_message_from_cjson(cJSON* root);
void discord_model_message_free(discord_message_t* message);

#ifdef __cplusplus
}
#endif