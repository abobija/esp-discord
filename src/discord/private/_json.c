#include "discord/private/_json.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "discord/private/_discord.h"
#include "cutils.h"
#include "estr.h"

DISCORD_LOG_DEFINE_BASE();

static struct {
    const char* name;
    discord_event_t event;
} discord_event_name_map[] = {
    { "READY",                    DISCORD_EVENT_READY },
    { "MESSAGE_CREATE",           DISCORD_EVENT_MESSAGE_RECEIVED },
    { "MESSAGE_DELETE",           DISCORD_EVENT_MESSAGE_DELETED },
    { "MESSAGE_UPDATE",           DISCORD_EVENT_MESSAGE_UPDATED },
    { "MESSAGE_REACTION_ADD",     DISCORD_EVENT_MESSAGE_REACTION_ADDED },
    { "MESSAGE_REACTION_REMOVE",  DISCORD_EVENT_MESSAGE_REACTION_REMOVED },
    { "VOICE_STATE_UPDATE",       DISCORD_EVENT_VOICE_STATE_UPDATED },
};

static discord_event_t discord_model_event_by_name(const char* name) {
    size_t map_len = sizeof(discord_event_name_map) / sizeof(discord_event_name_map[0]);

    for(size_t i = 0; i < map_len; i++) {
        if(estr_eq(name, discord_event_name_map[i].name)) {
            return discord_event_name_map[i].event;
        }
    }

    return DISCORD_EVENT_UNKNOWN;
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

discord_payload_t* discord_payload_from_cjson(cJSON* cjson) {
    discord_payload_t* pl = cu_ctor(discord_payload_t,
        .op = cJSON_GetObjectItem(cjson, "op")->valueint
    );

    // todo: memcheck

    cJSON* s = cJSON_GetObjectItem(cjson, "s");

    pl->s = cJSON_IsNumber(s) ? s->valueint : DISCORD_NULL_SEQUENCE_NUMBER;
    
    if(pl->s <= 0) {
        pl->s = DISCORD_NULL_SEQUENCE_NUMBER;
    }

    cJSON* d = cJSON_GetObjectItem(cjson, "d");

    switch(pl->op) {
        case DISCORD_OP_HELLO:
            pl->d = cu_ctor(discord_hello_t, .heartbeat_interval = cJSON_GetObjectItem(d, "heartbeat_interval")->valueint);
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

    return pl;
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
        
        case DISCORD_EVENT_VOICE_STATE_UPDATED:
            return discord_voice_state_from_cjson(cjson);

        default:
            DISCORD_LOGW("Cannot recognize event type");
            return NULL;
    }
}

cJSON* discord_heartbeat_to_cjson(discord_heartbeat_t* heartbeat) {
    int hb = *((int*) (heartbeat));

    // todo: memcheck
    return hb == DISCORD_NULL_SEQUENCE_NUMBER ? cJSON_CreateNull() : cJSON_CreateNumber(hb);
}

cJSON* discord_identify_properties_to_cjson(discord_identify_properties_t* properties) {
    cJSON* root = cJSON_CreateObject();

    // todo: memchecks
    cJSON_AddItemToObject(root, "os", cJSON_CreateStringReference(properties->os));
    cJSON_AddItemToObject(root, "browser", cJSON_CreateStringReference(properties->browser));
    cJSON_AddItemToObject(root, "device", cJSON_CreateStringReference(properties->device));

    return root;
}

cJSON* discord_identify_to_cjson(discord_identify_t* identify) {
    cJSON* root = cJSON_CreateObject();

    // todo: memchecks
    cJSON_AddItemToObject(root, "token", cJSON_CreateStringReference(identify->token));
    cJSON_AddNumberToObject(root, "intents", identify->intents);
    cJSON_AddItemToObject(root, "properties", discord_identify_properties_to_cjson(identify->properties));

    return root;
}

discord_session_t* discord_session_from_cjson(cJSON* root) {
    if(!root)
        return NULL;

    cJSON* _id = cJSON_GetObjectItem(root, "session_id");

    discord_session_t* session = cu_ctor(discord_session_t,
        .session_id = _id->valuestring,
        .user = discord_user_from_cjson(cJSON_GetObjectItem(root, "user"))
    );

    // todo: memcheck

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

    discord_user_t* user = cu_ctor(discord_user_t,
        .id = _id->valuestring,
        .bot = _bot && _bot->valueint,
        .username = _username->valuestring,
        .discriminator = _discriminator->valuestring
    );

    // todo: memcheck

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

    // todo: memchecks

    return root;
}

discord_member_t* discord_member_from_cjson(cJSON* root) {
    if(!root)
        return NULL;

    cJSON* _nick = cJSON_GetObjectItem(root, "nick");
    cJSON* _permissions = cJSON_GetObjectItem(root, "permissions");

    discord_member_t* member = cu_ctor(discord_member_t,
        .nick = _nick ? _nick->valuestring : NULL,
        .permissions = _permissions ? _permissions->valuestring : NULL
    );

    // todo: memcheck

    if(_nick) _nick->valuestring = NULL;
    if(_permissions) _permissions->valuestring = NULL;

    cJSON* _roles = cJSON_GetObjectItem(root, "roles");

    if(cJSON_IsArray(_roles) && ((member->_roles_len = cJSON_GetArraySize(_roles)) > 0)) {
        member->roles = calloc(member->_roles_len, sizeof(char*));

        // todo: memcheck

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

    // todo: memchecks

    return root;
}

discord_attachment_t* discord_attachment_from_cjson(cJSON* root) {
    if(!root)
        return NULL;

    cJSON* _id = cJSON_GetObjectItem(root, "id");
    cJSON* _fname = cJSON_GetObjectItem(root, "filename");
    cJSON* _ctype = cJSON_GetObjectItem(root, "content_type");
    cJSON* _url = cJSON_GetObjectItem(root, "url");

    discord_attachment_t* attachment = cu_ctor(discord_attachment_t,
        .id = _id->valuestring,
        .filename = _fname->valuestring,
        .content_type = _ctype ? _ctype->valuestring : NULL,
        .size = cJSON_GetObjectItem(root, "size")->valueint,
        .url = _url->valuestring
    );

    // todo: memcheck

    _id->valuestring =
    _fname->valuestring =
    _url->valuestring =
    NULL;

    if(_ctype != NULL) { _ctype->valuestring = NULL; }

    return attachment;
}

cJSON* discord_attachment_to_cjson(discord_attachment_t* attachment) {
    if(!attachment)
        return NULL;
    
    cJSON* root = cJSON_CreateObject();

    if(attachment->id) cJSON_AddItemToObject(root, "id", cJSON_CreateStringReference(attachment->id));
    if(attachment->filename) cJSON_AddItemToObject(root, "filename", cJSON_CreateStringReference(attachment->filename));

    // todo: memchecks

    return root;
}

static cJSON* discord_embed_footer_to_cjson(discord_embed_footer_t* footer)
{
    if(!footer)
        return NULL;
    
    cJSON* root = cJSON_CreateObject();

    if(footer->text) cJSON_AddItemToObject(root, "text", cJSON_CreateStringReference(footer->text));
    if(footer->icon_url) cJSON_AddItemToObject(root, "icon_url", cJSON_CreateStringReference(footer->icon_url));

    return root;
}

static cJSON* discord_embed_image_to_cjson(discord_embed_image_t* image)
{
    if(!image)
        return NULL;
    
    cJSON* root = cJSON_CreateObject();

    if(image->url) cJSON_AddItemToObject(root, "url", cJSON_CreateStringReference(image->url));

    return root;
}

static cJSON* discord_embed_author_to_cjson(discord_embed_author_t* author)
{
    if(!author)
        return NULL;
    
    cJSON* root = cJSON_CreateObject();

    if(author->name) cJSON_AddItemToObject(root, "name", cJSON_CreateStringReference(author->name));
    if(author->url) cJSON_AddItemToObject(root, "url", cJSON_CreateStringReference(author->url));
    if(author->icon_url) cJSON_AddItemToObject(root, "icon_url", cJSON_CreateStringReference(author->icon_url));

    return root;
}

static cJSON* discord_embed_field_to_cjson(discord_embed_field_t* field)
{
    if(!field)
        return NULL;
    
    cJSON* root = cJSON_CreateObject();

    if(field->name) cJSON_AddItemToObject(root, "name", cJSON_CreateStringReference(field->name));
    if(field->value) cJSON_AddItemToObject(root, "value", cJSON_CreateStringReference(field->value));
    cJSON_AddBoolToObject(root, "inline", field->is_inline);

    return root;
}

cJSON* discord_embed_to_cjson(discord_embed_t* embed)
{
    if(!embed)
        return NULL;
    
    cJSON* root = cJSON_CreateObject();

    if(embed->title) cJSON_AddItemToObject(root, "title", cJSON_CreateStringReference(embed->title));
    if(embed->description) cJSON_AddItemToObject(root, "description", cJSON_CreateStringReference(embed->description));
    if(embed->url) cJSON_AddItemToObject(root, "url", cJSON_CreateStringReference(embed->url));
    cJSON_AddNumberToObject(root, "color", embed->color);
    if(embed->footer) cJSON_AddItemToObject(root, "footer", discord_embed_footer_to_cjson(embed->footer));
    if(embed->image) cJSON_AddItemToObject(root, "image", discord_embed_image_to_cjson(embed->image));
    if(embed->thumbnail) cJSON_AddItemToObject(root, "thumbnail", discord_embed_image_to_cjson(embed->thumbnail));
    if(embed->author) cJSON_AddItemToObject(root, "author", discord_embed_author_to_cjson(embed->author));

    if(embed->_fields_len > 0) {
        cJSON* fields = cJSON_CreateArray();

        for(uint8_t i = 0; i < embed->_fields_len; i++) {
            cJSON_AddItemToArray(fields, discord_embed_field_to_cjson(embed->fields[i]));
        }

        cJSON_AddItemToObject(root, "fields", fields);
    }

    return root;
}

discord_guild_t* discord_guild_from_cjson(cJSON* root) {
    if(!root)
        return NULL;

    cJSON* _id = cJSON_GetObjectItem(root, "id");
    cJSON* _name = cJSON_GetObjectItem(root, "name");
    cJSON* _permissions = cJSON_GetObjectItem(root, "permissions");

    discord_guild_t* guild = cu_ctor(discord_guild_t,
        .id = _id->valuestring,
        .name = _name->valuestring,
        .permissions = _permissions == NULL ? NULL : _permissions->valuestring
    );

    // todo: memcheck

    _id->valuestring = _name->valuestring = NULL;

    if(_permissions) {
        _permissions->valuestring = NULL;
    }
    
    return guild;
}

cJSON* discord_guild_to_cjson(discord_guild_t* guild) {
    if(!guild)
        return NULL;
    
    cJSON* root = cJSON_CreateObject();

    cJSON_AddItemToObject(root, "id", cJSON_CreateStringReference(guild->id));
    cJSON_AddItemToObject(root, "name", cJSON_CreateStringReference(guild->name));
    
    if(guild->permissions) {
        cJSON_AddItemToObject(root, "permissions", cJSON_CreateStringReference(guild->permissions));
    }
    
    // todo: memchecks

    return root;
}

discord_channel_t* discord_channel_from_cjson(cJSON* root) {
    if(!root)
        return NULL;

    cJSON* _id = cJSON_GetObjectItem(root, "id");
    cJSON* _type = cJSON_GetObjectItem(root, "type");
    cJSON* _name = cJSON_GetObjectItem(root, "name");

    discord_channel_t* channel = cu_ctor(discord_channel_t,
        .id = _id->valuestring,
        .type = (discord_channel_type_t) _type->valueint,
        .name = _name == NULL ? NULL : _name->valuestring,
    );

    // todo: memcheck

    _id->valuestring = NULL;

    if(_name) {
        _name->valuestring = NULL;
    }
    
    return channel;
}

cJSON* discord_channel_to_cjson(discord_channel_t* channel) {
    if(!channel)
        return NULL;
    
    cJSON* root = cJSON_CreateObject();

    cJSON_AddItemToObject(root, "id", cJSON_CreateStringReference(channel->id));
    cJSON_AddItemToObject(root, "type", cJSON_CreateNumber(channel->type));
    
    if(channel->name) {
        cJSON_AddItemToObject(root, "name", cJSON_CreateStringReference(channel->name));
    }

    // todo: memchecks

    return root;
}

discord_role_t* discord_role_from_cjson(cJSON* root) {
    if(!root)
        return NULL;

    cJSON* _id = cJSON_GetObjectItem(root, "id");
    cJSON* _name = cJSON_GetObjectItem(root, "name");
    cJSON* _pos = cJSON_GetObjectItem(root, "position");
    cJSON* _permissions = cJSON_GetObjectItem(root, "permissions");

    discord_role_t* role = cu_ctor(discord_role_t,
        .id = _id->valuestring,
        .name = _name->valuestring,
        .position = _pos->valueint,
        .permissions = _permissions->valuestring
    );

    // todo: memcheck

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

    // todo: memchecks

    return root;
}

discord_message_t* discord_message_from_cjson(cJSON* root) {
    if(!root)
        return NULL;

    cJSON* _id = cJSON_GetObjectItem(root, "id");
    cJSON* _content = cJSON_GetObjectItem(root, "content");
    cJSON* _cid = cJSON_GetObjectItem(root, "channel_id");
    cJSON* _gid = cJSON_GetObjectItem(root, "guild_id");
    cJSON* _type = cJSON_GetObjectItem(root, "type");

    discord_message_t* message = cu_ctor(discord_message_t,
        .id = _id ? _id->valuestring : NULL,
        .type = (discord_message_type_t) (_type ? _type->valueint : DISCORD_MESSAGE_UNDEFINED),
        .content = _content ? _content->valuestring : NULL,
        .channel_id = _cid->valuestring,
        .author = discord_user_from_cjson(cJSON_GetObjectItem(root, "author")),
        .guild_id = _gid ? _gid->valuestring : NULL,
        .member = discord_member_from_cjson(cJSON_GetObjectItem(root, "member"))
    );

    // todo: memchecks

    _cid->valuestring = NULL;

    if(_content) _content->valuestring = NULL;
    if(_id) _id->valuestring = NULL;
    if(_gid) _gid->valuestring = NULL;

    cJSON* _attachments = cJSON_GetObjectItem(root, "attachments");

    if(cJSON_IsArray(_attachments) && ((message->_attachments_len = cJSON_GetArraySize(_attachments)) > 0)) {
        message->attachments = calloc(message->_attachments_len, sizeof(discord_attachment_t*));

        // todo: memcheck

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

    if(msg->_attachments_len > 0 && msg->attachments) {
        cJSON* attachments = cJSON_CreateArray();

        for(uint8_t i = 0; i < msg->_attachments_len; i++) {
            cJSON_AddItemToArray(attachments, discord_attachment_to_cjson(msg->attachments[i]));
        }

        cJSON_AddItemToObject(root, "attachments", attachments);
    }

    if(msg->_embeds_len > 0 && msg->embeds) {
        cJSON* embeds = cJSON_CreateArray();

        for(uint8_t i = 0; i < msg->_embeds_len; i++) {
            cJSON_AddItemToArray(embeds, discord_embed_to_cjson(msg->embeds[i]));
        }

        cJSON_AddItemToObject(root, "embeds", embeds);
    }

    // todo: memchecks

    return root;
}

discord_emoji_t* discord_emoji_from_cjson(cJSON* root) {
    if(!root)
        return NULL;

    cJSON* _name = cJSON_GetObjectItem(root, "name");

    if(!_name) {
        DISCORD_LOGW("Missing name");
        return NULL;
    }

    discord_emoji_t* emoji = cu_ctor(discord_emoji_t, .name = _name->valuestring);

    // todo: memcheck

    _name->valuestring = NULL;

    return emoji;
}

discord_message_reaction_t* discord_message_reaction_from_cjson(cJSON* root) {
    if(!root)
        return NULL;

    cJSON* _uid = cJSON_GetObjectItem(root, "user_id");
    cJSON* _mid = cJSON_GetObjectItem(root, "message_id");
    cJSON* _cid = cJSON_GetObjectItem(root, "channel_id");

    discord_message_reaction_t* react = cu_ctor(discord_message_reaction_t,
        .user_id = _uid->valuestring,
        .message_id = _mid->valuestring,
        .channel_id = _cid->valuestring,
        .emoji = discord_emoji_from_cjson(cJSON_GetObjectItem(root, "emoji"))
    );

    // todo: memcheck

    _uid->valuestring =
    _mid->valuestring =
    _cid->valuestring =
    NULL;

    return react;
}

discord_voice_state_t* discord_voice_state_from_cjson(cJSON* root) {
    if(!root)
        return NULL;

    cJSON* _gid = cJSON_GetObjectItem(root, "guild_id");
    cJSON* _cid = cJSON_GetObjectItem(root, "channel_id");
    cJSON* _uid = cJSON_GetObjectItem(root, "user_id");
    cJSON* _member = cJSON_GetObjectItem(root, "member");

    discord_voice_state_t* state = cu_ctor(discord_voice_state_t,
        .guild_id    = _gid->valuestring,
        .channel_id  = _cid ? _cid->valuestring : NULL,
        .user_id     = _uid->valuestring,
        .member      = discord_member_from_cjson(_member),
        .deaf        = (bool) cJSON_GetObjectItem(root, "deaf")->valueint,
        .mute        = (bool) cJSON_GetObjectItem(root, "mute")->valueint,
        .self_deaf   = (bool) cJSON_GetObjectItem(root, "self_deaf")->valueint,
        .self_mute   = (bool) cJSON_GetObjectItem(root, "self_mute")->valueint
    );

    // todo: memcheck

    _gid->valuestring =
    _uid->valuestring =
    NULL;

    if(_cid) { _cid->valuestring = NULL; }

    return state;
}