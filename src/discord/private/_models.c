#include "esp_heap_caps.h"
#include "cJSON.h"
#include "esp_log.h"
#include "discord/private/_discord.h"
#include "discord/private/_models.h"
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

discord_payload_t* discord_payload_ctor(int op, discord_payload_data_t d) {
    discord_payload_t* pl = calloc(1, sizeof(discord_payload_t));

    pl->op = op;
    pl->d = d;
    pl->s = DISCORD_NULL_SEQUENCE_NUMBER;
    pl->t = DISCORD_EVENT_NONE;

    return pl;
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

void discord_payload_free(discord_payload_t* payload) {
    if(!payload)
        return;

    switch (payload->op) {
        case DISCORD_OP_HELLO:
            discord_hello_free((discord_hello_t*) payload->d);
            break;

        case DISCORD_OP_DISPATCH:
            discord_dispatch_event_data_free(payload);
            break;

        case DISCORD_OP_HEARTBEAT:
        case DISCORD_OP_HEARTBEAT_ACK:
            // Ignore
            break;

        case DISCORD_OP_IDENTIFY:
            discord_identify_free((discord_identify_t*) payload->d);
            break;
        
        default:
            DISCORD_LOGW("Cannot recognize payload type. Possible memory leak.");
            break;
    }

    free(payload);
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
    discord_payload_free(payload);

    char* payload_raw = cJSON_PrintUnformatted(cjson);
    cJSON_Delete(cjson);

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

void discord_dispatch_event_data_free(discord_payload_t* payload) {
    if(!payload)
        return;

    switch (payload->t) {
        case DISCORD_EVENT_READY:
            return discord_session_free((discord_session_t*) payload->d);
        
        case DISCORD_EVENT_MESSAGE_RECEIVED:
        case DISCORD_EVENT_MESSAGE_UPDATED:
        case DISCORD_EVENT_MESSAGE_DELETED:
            return discord_message_free((discord_message_t*) payload->d);

        case DISCORD_EVENT_MESSAGE_REACTION_ADDED:
        case DISCORD_EVENT_MESSAGE_REACTION_REMOVED:
            return discord_message_reaction_free((discord_message_reaction_t*) payload->d);
        
        default:
            DISCORD_LOGW("Cannot recognize event type");
            return;
    }
}

cJSON* discord_heartbeat_to_cjson(discord_heartbeat_t* heartbeat) {
    int hb = *((int*) (heartbeat));

    return hb == DISCORD_NULL_SEQUENCE_NUMBER ? cJSON_CreateNull() : cJSON_CreateNumber(hb);
}

discord_hello_t* discord_hello_ctor(int heartbeat_interval) {
    discord_hello_t* hello = calloc(1, sizeof(discord_hello_t));

    hello->heartbeat_interval = heartbeat_interval;

    return hello;
}

void discord_hello_free(discord_hello_t* hello) {
    if(!hello)
        return;

    free(hello);
}

discord_identify_properties_t* discord_identify_properties_ctor(const char* $os, const char* $browser, const char* $device) {
    discord_identify_properties_t* props = calloc(1, sizeof(discord_identify_properties_t));

    props->$os = strdup($os);
    props->$browser = strdup($browser);
    props->$device = strdup($device);

    return props;
}

cJSON* discord_identify_properties_to_cjson(discord_identify_properties_t* properties) {
    cJSON* root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "$os", properties->$os);
    cJSON_AddStringToObject(root, "$browser", properties->$browser);
    cJSON_AddStringToObject(root, "$device", properties->$device);

    return root;
}

void discord_identify_properties_free(discord_identify_properties_t* properties) {
    if(!properties)
        return;

    free(properties->$os);
    free(properties->$browser);
    free(properties->$device);
    free(properties);
}

discord_identify_t* discord_identify_ctor(const char* token, int intents, discord_identify_properties_t* properties) {
    discord_identify_t* identify = calloc(1, sizeof(discord_identify_t));

    identify->token = strdup(token);
    identify->intents = intents;
    identify->properties = properties;

    return identify;
}

cJSON* discord_identify_to_cjson(discord_identify_t* identify) {
    cJSON* root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "token", identify->token);
    cJSON_AddNumberToObject(root, "intents", identify->intents);
    cJSON_AddItemToObject(root, "properties", discord_identify_properties_to_cjson(identify->properties));

    return root;
}

void discord_identify_free(discord_identify_t* identify) {
    if(!identify)
        return;

    free(identify->token);
    discord_identify_properties_free(identify->properties);
    free(identify);
}

discord_session_user_t* discord_session_user_from_cjson(cJSON* root) {
    if(!root)
        return NULL;
    
    discord_session_user_t* user = calloc(1, sizeof(discord_session_user_t));

    user->id = strdup(cJSON_GetObjectItem(root, "id")->valuestring);
    user->username = strdup(cJSON_GetObjectItem(root, "username")->valuestring);
    user->discriminator = strdup(cJSON_GetObjectItem(root, "discriminator")->valuestring);

    return user;
}

void discord_session_user_free(discord_session_user_t* user) {
    if(!user)
        return;
    
    free(user->id);
    free(user->username);
    free(user->discriminator);
    free(user);
}

discord_session_t* discord_session_from_cjson(cJSON* root) {
    if(!root)
        return NULL;
    
    discord_session_t* id = calloc(1, sizeof(discord_session_t));

    id->session_id = strdup(cJSON_GetObjectItem(root, "session_id")->valuestring);
    id->user = discord_session_user_from_cjson(cJSON_GetObjectItem(root, "user"));

    return id;
}

void discord_session_free(discord_session_t* id) {
    if(!id)
        return;

    discord_session_user_free(id->user);
    free(id->session_id);
    free(id);
}

discord_user_t* discord_user_from_cjson(cJSON* root) {
    if(!root)
        return NULL;
    
    discord_user_t* user = calloc(1, sizeof(discord_user_t));

    user->id = strdup(cJSON_GetObjectItem(root, "id")->valuestring);
    user->username = strdup(cJSON_GetObjectItem(root, "username")->valuestring);
    user->discriminator = strdup(cJSON_GetObjectItem(root, "discriminator")->valuestring);

    cJSON* bot = cJSON_GetObjectItem(root, "bot");
    user->bot = bot && bot->valueint;

    return user;
}

cJSON* discord_user_to_cjson(discord_user_t* user) {
    cJSON* root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "id", user->id);
    cJSON_AddStringToObject(root, "username", user->username);
    cJSON_AddStringToObject(root, "discriminator", user->discriminator);
    cJSON_AddBoolToObject(root, "bot", user->bot);

    return root;
}

discord_message_t* discord_message_ctor(const char* id, const char* content, const char* channel_id, discord_user_t* author) {
    discord_message_t* message = calloc(1, sizeof(discord_message_t));

    message->id = STRDUP(id);
    message->content = STRDUP(content);
    message->channel_id = STRDUP(channel_id);
    message->author = author;

    return message;
}

discord_message_t* discord_message_from_cjson(cJSON* root) {
    if(!root)
        return NULL;

    cJSON* _id = cJSON_GetObjectItem(root, "id");

    if(!_id) {
        DISCORD_LOGW("Missing id");
        return NULL;
    }

    cJSON* _content = cJSON_GetObjectItem(root, "content");
    cJSON* _chid = cJSON_GetObjectItem(root, "channel_id");

    return discord_message_ctor(
        _id->valuestring,
        _content ? _content->valuestring : NULL,
        _chid ? _chid->valuestring : NULL,
        discord_user_from_cjson(cJSON_GetObjectItem(root, "author"))
    );
}

cJSON* discord_message_to_cjson(discord_message_t* msg) {
    cJSON* root = cJSON_CreateObject();

    if(msg->id) cJSON_AddStringToObject(root, "id", msg->id);
    cJSON_AddStringToObject(root, "content", msg->content);
    cJSON_AddStringToObject(root, "channel_id", msg->channel_id);
    if(msg->author) cJSON_AddItemToObject(root, "author", discord_user_to_cjson(msg->author));

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

discord_emoji_t* discord_emoji_ctor(const char* name) {
    discord_emoji_t* emoji = calloc(1, sizeof(discord_emoji_t));

    emoji->name = STRDUP(name);

    return emoji;
}

discord_emoji_t* discord_emoji_from_cjson(cJSON* root) {
    if(!root)
        return NULL;

    cJSON* _name = cJSON_GetObjectItem(root, "name");

    if(!_name) {
        DISCORD_LOGW("Missing name");
        return NULL;
    }

    return discord_emoji_ctor(_name->valuestring);
}

discord_message_reaction_t* discord_message_reaction_ctor(const char* user_id, const char* message_id, const char* channel_id, discord_emoji_t* emoji) {
    discord_message_reaction_t* react = calloc(1, sizeof(discord_message_reaction_t));

    react->user_id = STRDUP(user_id);
    react->message_id = STRDUP(message_id);
    react->channel_id = STRDUP(channel_id);
    react->emoji = emoji;

    return react;
}

discord_message_reaction_t* discord_message_reaction_from_cjson(cJSON* root) {
    if(!root)
        return NULL;

    cJSON* _uid = cJSON_GetObjectItem(root, "user_id");
    cJSON* _mid = cJSON_GetObjectItem(root, "message_id");
    cJSON* _cid = cJSON_GetObjectItem(root, "channel_id");

    return discord_message_reaction_ctor(
        _uid ? _uid->valuestring : NULL,
        _mid ? _mid->valuestring : NULL,
        _cid ? _cid->valuestring : NULL,
        discord_emoji_from_cjson(cJSON_GetObjectItem(root, "emoji"))
    );
}