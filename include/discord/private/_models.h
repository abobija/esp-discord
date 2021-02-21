#ifndef _DISCORD_PRIVATE_MODELS_H_
#define _DISCORD_PRIVATE_MODELS_H_

#include "cJSON.h"
#include "discord/models/user.h"
#include "discord/models/message.h"
#include "discord/models/message_reaction.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DISCORD_NULL_SEQUENCE_NUMBER -1

typedef enum {
    DISCORD_GATEWAY_EVENT_UNKNOWN = -1,
    DISCORD_GATEWAY_EVENT_NONE,
    DISCORD_GATEWAY_EVENT_READY,
    DISCORD_GATEWAY_EVENT_MESSAGE_CREATE,
    DISCORD_GATEWAY_EVENT_MESSAGE_DELETE,
    DISCORD_GATEWAY_EVENT_MESSAGE_UPDATE,
    DISCORD_GATEWAY_EVENT_MESSAGE_REACTION_ADD
} discord_gateway_event_t;

typedef void* discord_gateway_payload_data_t;

typedef struct {
    int op;
    discord_gateway_payload_data_t d;
    int s;
    discord_gateway_event_t t;
} discord_gateway_payload_t;

typedef struct {
    int heartbeat_interval;
} discord_gateway_hello_t;

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

discord_gateway_payload_t* discord_model_gateway_payload(int op, discord_gateway_payload_data_t d);
cJSON* discord_model_gateway_payload_to_cjson(discord_gateway_payload_t* payload);
void discord_model_gateway_payload_free(discord_gateway_payload_t* payload);
discord_gateway_payload_t* discord_model_gateway_payload_deserialize(const char* json, size_t length);
/**
 * @brief Serialize payload to JSON string. WARNING: Payload will be automatically freed.
 */
char* discord_model_gateway_payload_serialize(discord_gateway_payload_t* payload);

discord_gateway_event_t discord_model_gateway_dispatch_event_name_map(const char* name);
discord_gateway_payload_data_t discord_model_gateway_dispatch_event_data_from_cjson(discord_gateway_event_t e, cJSON* cjson);
void discord_model_gateway_dispatch_event_data_free(discord_gateway_payload_t* payload);

discord_gateway_hello_t* discord_model_gateway_hello(int heartbeat_interval);
void discord_model_gateway_hello_free(discord_gateway_hello_t* hello);

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
cJSON* discord_model_user_to_cjson(discord_user_t* user);

discord_message_t* discord_model_message(const char* id, const char* content, const char* channel_id, discord_user_t* author);
discord_message_t* discord_model_message_from_cjson(cJSON* root);
cJSON* discord_model_message_to_cjson(discord_message_t* msg);
char* discord_model_message_serialize(discord_message_t* msg);
discord_message_t* discord_model_message_deserialize(const char* json, size_t length);

discord_emoji_t* discord_model_emoji(const char* name);
discord_emoji_t* discord_model_emoji_from_cjson(cJSON* root);

discord_message_reaction_t* discord_model_message_reaction(const char* user_id, const char* message_id, const char* channel_id, discord_emoji_t* emoji);
discord_message_reaction_t* discord_model_message_reaction_from_cjson(cJSON* root);

#ifdef __cplusplus
}
#endif

#endif