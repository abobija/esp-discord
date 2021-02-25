#include "discord/private/_json.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "discord/private/_discord.h"
#include "discord/utils.h"

DISCORD_LOG_DEFINE_BASE();

static struct {
    const char* name;
    discord_event_t event;
} discord_event_name_map[] = {
    { "READY",                   DISCORD_EVENT_READY },
    { "MESSAGE_CREATE",          DISCORD_EVENT_MESSAGE_RECEIVED },
    { "MESSAGE_DELETE",          DISCORD_EVENT_MESSAGE_DELETED },
    { "MESSAGE_UPDATE",          DISCORD_EVENT_MESSAGE_UPDATED },
    { "MESSAGE_REACTION_ADD",    DISCORD_EVENT_MESSAGE_REACTION_ADDED },
    { "MESSAGE_REACTION_REMOVE", DISCORD_EVENT_MESSAGE_REACTION_REMOVED }
};

static discord_event_t discord_model_event_by_name(const char* name) {
    size_t map_len = sizeof(discord_event_name_map) / sizeof(discord_event_name_map[0]);

    for(size_t i = 0; i < map_len; i++) {
        if(discord_streq(name, discord_event_name_map[i].name)) {
            return discord_event_name_map[i].event;
        }
    }

    return DISCORD_EVENT_UNKNOWN;
}

static cJSON* _discord_model_parse(const char* json, size_t length) {
    cJSON* cjson = cJSON_ParseWithLength(json, length);
    
    if(!cjson) {
        DISCORD_LOGW("JSON parsing (syntax?) error");
        return NULL;
    }

    return cjson;
}

cJSON* discord_payload_to_cjson(discord_payload_t* payload) {
    if(!payload)
        return NULL;
    
    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "op", payload->op);

    const char* d = "d";

    switch (payload->op) {
        case DISCORD_OP_HEARTBEAT:
            cJSON_AddItemToObject(root, d, discord_heartbeat_to_cjson((discord_heartbeat_t*) payload->d));
            break;

        case DISCORD_OP_IDENTIFY:
            cJSON_AddItemToObject(root, d, discord_identify_to_cjson((discord_identify_t*) payload->d));
            break;
        
        default:
            DISCORD_LOGW("Cannot recognize payload type");
            cJSON_Delete(root);
            root = NULL;
            break;
    }

    return root;
}

discord_payload_t* discord_payload_deserialize(const char* json, size_t length) {
    cJSON* cjson = _discord_model_parse(json, length);

    if(!cjson)
        return NULL;

    discord_payload_t* pl = discord_payload_ctor(
        cJSON_GetObjectItem(cjson, "op")->valueint,
        NULL
    );

    cJSON* s = cJSON_GetObjectItem(cjson, "s");

    pl->s = cJSON_IsNumber(s) ? s->valueint : DISCORD_NULL_SEQUENCE_NUMBER;
    
    if(pl->s <= 0) {
        pl->s = DISCORD_NULL_SEQUENCE_NUMBER;
    }

    cJSON* d = cJSON_GetObjectItem(cjson, "d");

    switch(pl->op) {
        case DISCORD_OP_HELLO:
            pl->d = discord_hello_ctor(cJSON_GetObjectItem(d, "heartbeat_interval")->valueint);
            break;

        case DISCORD_OP_DISPATCH:
            pl->t = discord_model_event_by_name(cJSON_GetObjectItem(cjson, "t")->valuestring);
            pl->d = discord_dispatch_event_data_from_cjson(pl->t, d);
            break;

        case DISCORD_OP_HEARTBEAT_ACK:
            // Ignore
            break;
        
        default:
            DISCORD_LOGW("Cannot recognize payload type. Unable to set payload data.");
            break;
    }

    cJSON_Delete(cjson);

    return pl;
}

char* discord_payload_serialize(discord_payload_t* payload) {
    cJSON* cjson = discord_payload_to_cjson(payload);
    char* payload_raw = cJSON_PrintUnformatted(cjson);
    cJSON_Delete(cjson);

    discord_payload_free(payload); // free after parsing bcs strings are added to cjson by reference

    return payload_raw;
}

discord_payload_data_t discord_dispatch_event_data_from_cjson(discord_event_t e, cJSON* cjson) {
    switch (e) {
        case DISCORD_EVENT_READY:
            return discord_session_from_cjson(cjson);
        
        case DISCORD_EVENT_MESSAGE_RECEIVED:
        case DISCORD_EVENT_MESSAGE_UPDATED:
        case DISCORD_EVENT_MESSAGE_DELETED:
            return discord_message_from_cjson(cjson);

        case DISCORD_EVENT_MESSAGE_REACTION_ADDED:
        case DISCORD_EVENT_MESSAGE_REACTION_REMOVED:
            return discord_message_reaction_from_cjson(cjson);
        
        default:
            DISCORD_LOGW("Cannot recognize event type");
            return NULL;
    }
}

cJSON* discord_heartbeat_to_cjson(discord_heartbeat_t* heartbeat) {
    int hb = *((int*) (heartbeat));

    return hb == DISCORD_NULL_SEQUENCE_NUMBER ? cJSON_CreateNull() : cJSON_CreateNumber(hb);
}

cJSON* discord_identify_properties_to_cjson(discord_identify_properties_t* properties) {
    cJSON* root = cJSON_CreateObject();

    cJSON_AddItemToObject(root, "$os", cJSON_CreateStringReference(properties->$os));
    cJSON_AddItemToObject(root, "$browser", cJSON_CreateStringReference(properties->$browser));
    cJSON_AddItemToObject(root, "$device", cJSON_CreateStringReference(properties->$device));

    return root;
}

cJSON* discord_identify_to_cjson(discord_identify_t* identify) {
    cJSON* root = cJSON_CreateObject();

    cJSON_AddItemToObject(root, "token", cJSON_CreateStringReference(identify->token));
    cJSON_AddNumberToObject(root, "intents", identify->intents);
    cJSON_AddItemToObject(root, "properties", discord_identify_properties_to_cjson(identify->properties));

    return root;
}

discord_session_t* discord_session_from_cjson(cJSON* root) {
    if(!root)
        return NULL;

    cJSON* _id = cJSON_GetObjectItem(root, "session_id");

    discord_session_t* session = discord_session_ctor(
        _id->valuestring,
        discord_user_from_cjson(cJSON_GetObjectItem(root, "user"))
    );

    _id->valuestring = NULL;

    return session;
}

discord_user_t* discord_user_from_cjson(cJSON* root) {
    if(!root)
        return NULL;

    cJSON* _id = cJSON_GetObjectItem(root, "id");
    cJSON* _bot = cJSON_GetObjectItem(root, "bot");
    cJSON* _username = cJSON_GetObjectItem(root, "username");
    cJSON* _discriminator = cJSON_GetObjectItem(root, "discriminator");

    discord_user_t* user = discord_user_ctor(
        _id->valuestring,
        _bot && _bot->valueint,
        _username->valuestring,
        _discriminator->valuestring
    );

    _id->valuestring =
    _username->valuestring =
    _discriminator->valuestring =
    NULL;

    return user;
}

cJSON* discord_user_to_cjson(discord_user_t* user) {
    cJSON* root = cJSON_CreateObject();

    cJSON_AddItemToObject(root, "id", cJSON_CreateStringReference(user->id));
    cJSON_AddItemToObject(root, "username", cJSON_CreateStringReference(user->username));
    cJSON_AddItemToObject(root, "discriminator", cJSON_CreateStringReference(user->discriminator));
    cJSON_AddBoolToObject(root, "bot", user->bot);

    return root;
}

discord_member_t* discord_member_from_cjson(cJSON* root) {
    if(!root)
        return NULL;

    cJSON* _nick = cJSON_GetObjectItem(root, "nick");
    cJSON* _permissions = cJSON_GetObjectItem(root, "permissions");

    discord_member_t* member = discord_member_ctor(
        _nick ? _nick->valuestring : NULL,
        _permissions ? _permissions->valuestring : NULL,
        NULL,
        0
    );

    if(_nick) _nick->valuestring = NULL;
    if(_permissions) _permissions->valuestring = NULL;

    cJSON* _roles = cJSON_GetObjectItem(root, "roles");

    if(cJSON_IsArray(_roles) && ((member->_roles_len = cJSON_GetArraySize(_roles)) > 0)) {
        member->roles = calloc(member->_roles_len, sizeof(char*));

        for(discord_role_len_t i = 0; i < member->_roles_len; i++) {
            cJSON* _role = cJSON_GetArrayItem(_roles, i);
            member->roles[i] = _role->valuestring;
            _role->valuestring = NULL;
        }
    }

    return member;
}

cJSON* discord_member_to_cjson(discord_member_t* member) {
    if(!member)
        return NULL;
    
    cJSON* root = cJSON_CreateObject();

    if(member->nick) cJSON_AddItemToObject(root, "nick", cJSON_CreateStringReference(member->nick));
    if(member->permissions) cJSON_AddItemToObject(root, "permissions", cJSON_CreateStringReference(member->permissions));

    return root;
}

discord_member_t* discord_member_deserialize(const char* json, size_t length) {
    cJSON* cjson = _discord_model_parse(json, length);

    if(!cjson)
        return NULL;

    discord_member_t* member = discord_member_from_cjson(cjson);
    cJSON_Delete(cjson);

    return member;
}

discord_attachment_t* discord_attachment_from_cjson(cJSON* root) {
    if(!root)
        return NULL;

    cJSON* _id = cJSON_GetObjectItem(root, "id");
    cJSON* _fname = cJSON_GetObjectItem(root, "filename");
    cJSON* _ctype = cJSON_GetObjectItem(root, "content_type");
    cJSON* _url = cJSON_GetObjectItem(root, "url");

    discord_attachment_t* attachment = discord_attachment_ctor(
        _id->valuestring,
        _fname->valuestring,
        _ctype->valuestring,
        cJSON_GetObjectItem(root, "size")->valueint,
        _url->valuestring
    );

    _id->valuestring =
    _fname->valuestring =
    _ctype->valuestring =
    _url->valuestring =
    NULL;

    return attachment;
}

discord_role_t* discord_role_from_cjson(cJSON* root) {
    if(!root)
        return NULL;

    cJSON* _id = cJSON_GetObjectItem(root, "id");
    cJSON* _name = cJSON_GetObjectItem(root, "name");
    cJSON* _pos = cJSON_GetObjectItem(root, "position");
    cJSON* _permissions = cJSON_GetObjectItem(root, "permissions");

    discord_role_t* role = discord_role_ctor(
        _id->valuestring,
        _name->valuestring,
        _pos->valueint,
        _permissions->valuestring
    );

    _id->valuestring =
    _name->valuestring =
    _permissions->valuestring =
    NULL;

    return role;
}

cJSON* discord_role_to_cjson(discord_role_t* role) {
    if(!role)
        return NULL;
    
    cJSON* root = cJSON_CreateObject();

    if(role->id) cJSON_AddItemToObject(root, "id", cJSON_CreateStringReference(role->id));
    cJSON_AddItemToObject(root, "name", cJSON_CreateStringReference(role->name));
    cJSON_AddNumberToObject(root, "position", role->position);
    cJSON_AddItemToObject(root, "permissions", cJSON_CreateStringReference(role->permissions));

    return root;
}

discord_role_t* discord_role_deserialize(const char* json, size_t length) {
    cJSON* cjson = _discord_model_parse(json, length);

    if(!cjson)
        return NULL;

    discord_role_t* role = discord_role_from_cjson(cjson);
    cJSON_Delete(cjson);

    return role;
}

discord_role_t** discord_role_list_deserialize(const char* json, size_t length, discord_role_len_t* roles_len) {
    cJSON* cjson = _discord_model_parse(json, length);

    if(!cJSON_IsArray(cjson))
        return NULL;

    discord_role_len_t _roles_len = cJSON_GetArraySize(cjson);
    discord_role_t** roles = calloc(_roles_len, sizeof(discord_role_t*));

    for(discord_role_len_t i = 0; i < _roles_len; i++) {
        roles[i] = discord_role_from_cjson(cJSON_GetArrayItem(cjson, i));
    }

    *roles_len = _roles_len;
    
    cJSON_Delete(cjson);
    
    return roles;
}

discord_message_t* discord_message_from_cjson(cJSON* root) {
    if(!root)
        return NULL;

    cJSON* _id = cJSON_GetObjectItem(root, "id");
    cJSON* _content = cJSON_GetObjectItem(root, "content");
    cJSON* _cid = cJSON_GetObjectItem(root, "channel_id");
    cJSON* _gid = cJSON_GetObjectItem(root, "guild_id");

    discord_message_t* message = discord_message_ctor(
        _id ? _id->valuestring : NULL,
        _content->valuestring,
        _cid->valuestring,
        discord_user_from_cjson(cJSON_GetObjectItem(root, "author")),
        _gid ? _gid->valuestring : NULL,
        discord_member_from_cjson(cJSON_GetObjectItem(root, "member")),
        NULL,
        0
    );

    _content->valuestring =
    _cid->valuestring =
    NULL;

    if(_id) _id->valuestring = NULL;
    if(_gid) _gid->valuestring = NULL;

    cJSON* _attachments = cJSON_GetObjectItem(root, "attachments");

    if(cJSON_IsArray(_attachments) && ((message->_attachments_len = cJSON_GetArraySize(_attachments)) > 0)) {
        message->attachments = calloc(message->_attachments_len, sizeof(discord_attachment_t*));

        for(uint8_t i = 0; i < message->_attachments_len; i++) {
            message->attachments[i] = discord_attachment_from_cjson(cJSON_GetArrayItem(_attachments, i));
        }
    }

    return message;
}

cJSON* discord_message_to_cjson(discord_message_t* msg) {
    cJSON* root = cJSON_CreateObject();

    if(msg->id) cJSON_AddItemToObject(root, "id", cJSON_CreateStringReference(msg->id));
    cJSON_AddItemToObject(root, "content", cJSON_CreateStringReference(msg->content));
    cJSON_AddItemToObject(root, "channel_id", cJSON_CreateStringReference(msg->channel_id));
    if(msg->author) cJSON_AddItemToObject(root, "author", discord_user_to_cjson(msg->author));
    if(msg->guild_id) cJSON_AddItemToObject(root, "guild_id", cJSON_CreateStringReference(msg->guild_id));
    if(msg->member) cJSON_AddItemToObject(root, "member", discord_member_to_cjson(msg->member));

    return root;
}


char* discord_message_serialize(discord_message_t* msg) {
    cJSON* cjson = discord_message_to_cjson(msg);
    char* payload_raw = cJSON_PrintUnformatted(cjson);
    cJSON_Delete(cjson);

    return payload_raw;
}

discord_message_t* discord_message_deserialize(const char* json, size_t length) {
    cJSON* cjson = _discord_model_parse(json, length);

    if(!cjson)
        return NULL;

    discord_message_t* msg = discord_message_from_cjson(cjson);
    cJSON_Delete(cjson);

    return msg;
}

discord_emoji_t* discord_emoji_from_cjson(cJSON* root) {
    if(!root)
        return NULL;

    cJSON* _name = cJSON_GetObjectItem(root, "name");

    if(!_name) {
        DISCORD_LOGW("Missing name");
        return NULL;
    }

    discord_emoji_t* emoji = discord_emoji_ctor(_name->valuestring);

    _name->valuestring = NULL;

    return emoji;
}

discord_message_reaction_t* discord_message_reaction_from_cjson(cJSON* root) {
    if(!root)
        return NULL;

    cJSON* _uid = cJSON_GetObjectItem(root, "user_id");
    cJSON* _mid = cJSON_GetObjectItem(root, "message_id");
    cJSON* _cid = cJSON_GetObjectItem(root, "channel_id");

    discord_message_reaction_t* react = discord_message_reaction_ctor(
        _uid->valuestring,
        _mid->valuestring,
        _cid->valuestring,
        discord_emoji_from_cjson(cJSON_GetObjectItem(root, "emoji"))
    );

    _uid->valuestring =
    _mid->valuestring =
    _cid->valuestring =
    NULL;

    return react;
}