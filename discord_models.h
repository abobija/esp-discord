#pragma once

#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char* id;
    char* username;
    char* discriminator;
} discord_gateway_session_user_t;

typedef struct {
    char* id;
    bool bot;
    char* username;
    char* discriminator;
} discord_user_t;

typedef struct {
    char* session_id;
    discord_gateway_session_user_t* user;
} discord_gateway_session_t;

discord_gateway_session_user_t* discord_model_gateway_session_user(cJSON* root);
void discord_model_gateway_session_user_free(discord_gateway_session_user_t* user);
discord_gateway_session_t* discord_model_gateway_session(cJSON* root);
void discord_model_gateway_session_free(discord_gateway_session_t* id);
discord_user_t* discord_model_user(cJSON* root);
void discord_model_user_free(discord_user_t* user);

#ifdef __cplusplus
}
#endif