#ifndef _DISCORD_PRIVATE_MODELS_H_
#define _DISCORD_PRIVATE_MODELS_H_

#include "cJSON.h"
#include "discord.h"
#include "discord/models/user.h"
#include "discord/models/message.h"
#include "discord/models/message_reaction.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DISCORD_NULL_SEQUENCE_NUMBER -1

typedef void* discord_payload_data_t;

typedef struct {
    int op;
    discord_payload_data_t d;
    int s;
    discord_event_t t;
} discord_payload_t;

typedef struct {
    int heartbeat_interval;
} discord_hello_t;

typedef int discord_heartbeat_t;

typedef struct {
    char* $os;
    char* $browser;
    char* $device;
} discord_identify_properties_t;

typedef struct {
    char* token;
    int intents;
    discord_identify_properties_t* properties;
} discord_identify_t;

typedef struct {
    char* id;
    char* username;
    char* discriminator;
} discord_session_user_t;

typedef struct {
    char* session_id;
    discord_session_user_t* user;
} discord_session_t;

discord_payload_t* discord_payload_ctor(int op, discord_payload_data_t d);
cJSON* discord_payload_to_cjson(discord_payload_t* payload);
void discord_payload_free(discord_payload_t* payload);
discord_payload_t* discord_payload_deserialize(const char* json, size_t length);
/**
 * @brief Serialize payload to JSON string. WARNING: Payload will be automatically freed.
 */
char* discord_payload_serialize(discord_payload_t* payload);

discord_payload_data_t discord_dispatch_event_data_from_cjson(discord_event_t e, cJSON* cjson);
void discord_dispatch_event_data_free(discord_payload_t* payload);

discord_hello_t* discord_hello_ctor(int heartbeat_interval);
void discord_hello_free(discord_hello_t* hello);

cJSON* discord_heartbeat_to_cjson(discord_heartbeat_t* heartbeat);

discord_identify_properties_t* discord_identify_properties_ctor(const char* $os, const char* $browser, const char* $device);
cJSON* discord_identify_properties_to_cjson(discord_identify_properties_t* properties);
void discord_identify_properties_free(discord_identify_properties_t* properties);

discord_identify_t* discord_identify_ctor(const char* token, int intents, discord_identify_properties_t* properties);
cJSON* discord_identify_to_cjson(discord_identify_t* identify);
void discord_identify_free(discord_identify_t* identify);

discord_session_user_t* discord_session_user_from_cjson(cJSON* root);
void discord_session_user_free(discord_session_user_t* user);

discord_session_t* discord_session_from_cjson(cJSON* root);
void discord_session_free(discord_session_t* id);

discord_user_t* discord_user_from_cjson(cJSON* root);
cJSON* discord_user_to_cjson(discord_user_t* user);

discord_message_t* discord_message_ctor(const char* id, const char* content, const char* channel_id, discord_user_t* author);
discord_message_t* discord_message_from_cjson(cJSON* root);
cJSON* discord_message_to_cjson(discord_message_t* msg);
char* discord_message_serialize(discord_message_t* msg);
discord_message_t* discord_message_deserialize(const char* json, size_t length);

discord_emoji_t* discord_emoji_ctor(const char* name);
discord_emoji_t* discord_emoji_from_cjson(cJSON* root);

discord_message_reaction_t* discord_message_reaction_ctor(const char* user_id, const char* message_id, const char* channel_id, discord_emoji_t* emoji);
discord_message_reaction_t* discord_message_reaction_from_cjson(cJSON* root);

#ifdef __cplusplus
}
#endif

#endif