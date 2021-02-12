#pragma once

#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DISCORD_MODEL_UNKNOWN,
    DISCORD_MODEL_HEARTBEAT,
    DISCORD_MODEL_GATEWAY_IDENTIFY
} discord_model_type_t;

typedef struct {
    int op;
    discord_model_type_t type;
    void* d;
} discord_gateway_payload_t;

typedef int discord_gateway_heartbeat_t;

typedef struct {
    char* $os;
    char* $browser;
    char* $device;
} discord_gateway_identify_properties_t;

typedef struct {
    char* token;
    int intents;
    discord_gateway_identify_properties_t* properties;
} discord_gateway_identify_t;

typedef struct {
    char* id;
    char* username;
    char* discriminator;
} discord_gateway_session_user_t;

typedef struct {
    char* session_id;
    discord_gateway_session_user_t* user;
} discord_gateway_session_t;

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

discord_gateway_payload_t* discord_model_gateway_payload(int op, discord_model_type_t type, void* d);
cJSON* discord_model_gateway_payload_to_cjson(discord_gateway_payload_t* payload);
void discord_model_gateway_payload_free(discord_gateway_payload_t* payload);

cJSON* discord_model_gateway_heartbeat_to_cjson(discord_gateway_heartbeat_t* heartbeat);

discord_gateway_identify_properties_t* discord_model_gateway_identify_properties(const char* $os, const char* $browser, const char* $device);
cJSON* discord_model_gateway_identify_properties_to_cjson(discord_gateway_identify_properties_t* properties);
void discord_model_gateway_identify_properties_free(discord_gateway_identify_properties_t* properties);

discord_gateway_identify_t* discord_model_gateway_identify(const char* token, int intents, discord_gateway_identify_properties_t* properties);
cJSON* discord_model_gateway_identify_to_cjson(discord_gateway_identify_t* identify);
void discord_model_gateway_identify_free(discord_gateway_identify_t* identify);

discord_gateway_session_user_t* discord_model_gateway_session_user_from_cjson(cJSON* root);
void discord_model_gateway_session_user_free(discord_gateway_session_user_t* user);

discord_gateway_session_t* discord_model_gateway_session_from_cjson(cJSON* root);
void discord_model_gateway_session_free(discord_gateway_session_t* id);

discord_user_t* discord_model_user_from_cjson(cJSON* root);
void discord_model_user_free(discord_user_t* user);

discord_message_t* discord_model_message_from_cjson(cJSON* root);
void discord_model_message_free(discord_message_t* message);

#ifdef __cplusplus
}
#endif