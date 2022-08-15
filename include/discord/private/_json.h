#ifndef _DISCORD_PRIVATE_JSON_H_
#define _DISCORD_PRIVATE_JSON_H_

#include "cJSON.h"
#include "discord/private/_models.h"
#include "discord/session.h"
#include "discord/user.h"
#include "discord/member.h"
#include "discord/message.h"
#include "discord/message_reaction.h"
#include "discord/guild.h"
#include "discord/channel.h"
#include "discord/role.h"
#include "discord/attachment.h"
#include "discord/voice_state.h"

#ifdef __cplusplus
extern "C" {
#endif

#define discord_json_serialize_(obj, to_cjson_fnc) \
    ({ cJSON* cjson = to_cjson_fnc(obj); char* json = cJSON_PrintUnformatted(cjson); cJSON_Delete(cjson); json; })

#define discord_json_serialize(obj) discord_json_serialize_(obj, discord_ ##obj ##_to_cjson)

#define discord_json_deserialize(type, from_cjson_fnc, json, length) ({ \
        type* obj = NULL; \
        cJSON* cjson = cJSON_ParseWithLength(json, length); \
        if(cjson) { obj = from_cjson_fnc(cjson); cJSON_Delete(cjson); } \
        else { DISCORD_LOGW("JSON parsing (syntax?) error"); } \
        obj; \
    })

#define discord_json_deserialize_(obj_name, json, length) \
    discord_json_deserialize(discord_ ##obj_name ##_t, discord_ ##obj_name ##_from_cjson, json, length)

#define discord_json_list_deserialize(type, from_cjson_fnc, json, length, out_length) ({ \
        cJSON* cjson = cJSON_ParseWithLength(json, length); \
        type** list = NULL; \
        if(!cjson) { DISCORD_LOGW("JSON parsing (syntax?) error"); } \
        else if(cJSON_IsArray(cjson)) { \
            int _len = cJSON_GetArraySize(cjson); \
            list = calloc(_len, sizeof(type*)); \
            if(list) { \
                for(int i = 0; i < _len; i++) { list[i] = from_cjson_fnc(cJSON_GetArrayItem(cjson, i)); } \
                if((out_length)) { *(out_length) = _len; } \
            } \
        } \
        cJSON_Delete(cjson); \
        list; \
    })

#define discord_json_list_deserialize_(obj_name, json, length, out_length) \
    discord_json_list_deserialize(discord_ ##obj_name ##_t, discord_ ##obj_name ##_from_cjson, json, length, out_length)

cJSON* discord_payload_to_cjson(discord_payload_t* payload);
discord_payload_t* discord_payload_from_cjson(cJSON* cjson);

discord_payload_data_t discord_dispatch_event_data_from_cjson(discord_event_t e, cJSON* cjson);

cJSON* discord_heartbeat_to_cjson(discord_heartbeat_t* heartbeat);

cJSON* discord_identify_properties_to_cjson(discord_identify_properties_t* properties);

cJSON* discord_identify_to_cjson(discord_identify_t* identify);

discord_session_t* discord_session_from_cjson(cJSON* root);

discord_user_t* discord_user_from_cjson(cJSON* root);
cJSON* discord_user_to_cjson(discord_user_t* user);

discord_member_t* discord_member_from_cjson(cJSON* root);
cJSON* discord_member_to_cjson(discord_member_t* member);

discord_attachment_t* discord_attachment_from_cjson(cJSON* root);
cJSON* discord_attachment_to_cjson(discord_attachment_t* attachment);

cJSON* discord_embed_to_cjson(discord_embed_t* embed);

discord_guild_t* discord_guild_from_cjson(cJSON* root);
cJSON* discord_guild_to_cjson(discord_guild_t* guild);

discord_channel_t* discord_channel_from_cjson(cJSON* root);
cJSON* discord_channel_to_cjson(discord_channel_t* channel);

discord_role_t* discord_role_from_cjson(cJSON* root);
cJSON* discord_role_to_cjson(discord_role_t* role);

discord_message_t* discord_message_from_cjson(cJSON* root);
cJSON* discord_message_to_cjson(discord_message_t* msg);

discord_emoji_t* discord_emoji_from_cjson(cJSON* root);

discord_message_reaction_t* discord_message_reaction_from_cjson(cJSON* root);

discord_voice_state_t* discord_voice_state_from_cjson(cJSON* root);

#ifdef __cplusplus
}
#endif

#endif