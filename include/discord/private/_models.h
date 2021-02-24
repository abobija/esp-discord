#ifndef _DISCORD_PRIVATE_MODELS_H_
#define _DISCORD_PRIVATE_MODELS_H_

#include "cJSON.h"
#include "discord.h"
#include "discord/session.h"
#include "discord/user.h"
#include "discord/member.h"
#include "discord/message.h"
#include "discord/message_reaction.h"
#include "discord/role.h"

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

discord_identify_properties_t* discord_identify_properties_ctor_(const char* $os, const char* $browser, const char* $device);
cJSON* discord_identify_properties_to_cjson(discord_identify_properties_t* properties);
void discord_identify_properties_free(discord_identify_properties_t* properties);

discord_identify_t* discord_identify_ctor_(const char* token, int intents, discord_identify_properties_t* properties);
cJSON* discord_identify_to_cjson(discord_identify_t* identify);
void discord_identify_free(discord_identify_t* identify);

discord_session_t* discord_session_ctor(char* id, discord_user_t* user);
discord_session_t* discord_session_clone(discord_session_t* session);
discord_session_t* discord_session_from_cjson(cJSON* root);

discord_user_t* discord_user_ctor(char* id, bool bot, char* username, char* discriminator);
discord_user_t* discord_user_clone(discord_user_t* user);
discord_user_t* discord_user_from_cjson(cJSON* root);
cJSON* discord_user_to_cjson(discord_user_t* user);

discord_member_t* discord_member_ctor(char* nick, char* permissions, char** roles, uint8_t roles_len);
discord_member_t* discord_member_from_cjson(cJSON* root);
cJSON* discord_member_to_cjson(discord_member_t* member);
discord_member_t* discord_member_deserialize(const char* json, size_t length);

discord_role_t* discord_role_ctor(char* id, char* name, uint8_t position, char* permissions);
discord_role_t* discord_role_from_cjson(cJSON* root);
cJSON* discord_role_to_cjson(discord_role_t* role);
discord_role_t* discord_role_deserialize(const char* json, size_t length);
discord_role_t** discord_role_list_deserialize(const char* json, size_t length, uint8_t* roles_len);

discord_message_t* discord_message_ctor(char* id, char* content, char* channel_id, discord_user_t* author, char* guild_id, discord_member_t* member);
discord_message_t* discord_message_from_cjson(cJSON* root);
cJSON* discord_message_to_cjson(discord_message_t* msg);
char* discord_message_serialize(discord_message_t* msg);
discord_message_t* discord_message_deserialize(const char* json, size_t length);

discord_emoji_t* discord_emoji_ctor(char* name);
discord_emoji_t* discord_emoji_from_cjson(cJSON* root);

discord_message_reaction_t* discord_message_reaction_ctor(char* user_id, char* message_id, char* channel_id, discord_emoji_t* emoji);
discord_message_reaction_t* discord_message_reaction_from_cjson(cJSON* root);

#ifdef __cplusplus
}
#endif

#endif