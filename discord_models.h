#pragma once

#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char* id;
    bool bot;
} discord_user_t;

typedef struct {
    char* session_id;
    discord_user_t* user;
} discord_gateway_identification_t;

discord_user_t* discord_model_user(cJSON* root);
void discord_model_user_free(discord_user_t* user);
discord_gateway_identification_t* discord_model_gateway_identification(cJSON* root);
void discord_model_gateway_identification_free(discord_gateway_identification_t* id);

#ifdef __cplusplus
}
#endif