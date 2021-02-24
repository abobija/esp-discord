#ifndef _DISCORD_PRIVATE_JSON_H_
#define _DISCORD_PRIVATE_JSON_H_

#include "cJSON.h"
#include "discord/private/_models.h"
#include "discord/session.h"
#include "discord/user.h"
#include "discord/member.h"
#include "discord/message.h"
#include "discord/message_reaction.h"
#include "discord/role.h"

#ifdef __cplusplus
extern "C" {
#endif

cJSON* discord_payload_to_cjson(discord_payload_t* payload);
discord_payload_t* discord_payload_deserialize(const char* json, size_t length);
/**
 * @brief Serialize payload to JSON string. WARNING: Payload will be automatically freed.
 */
char* discord_payload_serialize(discord_payload_t* payload);

discord_payload_data_t discord_dispatch_event_data_from_cjson(discord_event_t e, cJSON* cjson);

cJSON* discord_heartbeat_to_cjson(discord_heartbeat_t* heartbeat);

cJSON* discord_identify_properties_to_cjson(discord_identify_properties_t* properties);

cJSON* discord_identify_to_cjson(discord_identify_t* identify);

discord_session_t* discord_session_from_cjson(cJSON* root);

discord_user_t* discord_user_from_cjson(cJSON* root);
cJSON* discord_user_to_cjson(discord_user_t* user);

discord_member_t* discord_member_from_cjson(cJSON* root);
cJSON* discord_member_to_cjson(discord_member_t* member);
discord_member_t* discord_member_deserialize(const char* json, size_t length);

discord_role_t* discord_role_from_cjson(cJSON* root);
cJSON* discord_role_to_cjson(discord_role_t* role);
discord_role_t* discord_role_deserialize(const char* json, size_t length);
discord_role_t** discord_role_list_deserialize(const char* json, size_t length, uint8_t* roles_len);

discord_message_t* discord_message_from_cjson(cJSON* root);
cJSON* discord_message_to_cjson(discord_message_t* msg);
char* discord_message_serialize(discord_message_t* msg);
discord_message_t* discord_message_deserialize(const char* json, size_t length);

discord_emoji_t* discord_emoji_from_cjson(cJSON* root);

discord_message_reaction_t* discord_message_reaction_from_cjson(cJSON* root);

#ifdef __cplusplus
}
#endif

#endif