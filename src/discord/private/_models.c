#include "string.h"
#include "esp_heap_caps.h"
#include "cJSON.h"
#include "esp_log.h"
#include "discord/private/_discord.h"
#include "discord.h"
#include "discord/models.h"

DISCORD_LOG_DEFINE_BASE();

discord_gateway_payload_t* discord_model_gateway_payload(int op, discord_gateway_payload_data_t d) {
    discord_gateway_payload_t* pl = calloc(1, sizeof(discord_gateway_payload_t));

    pl->op = op;
    pl->d = d;
    pl->s = DISCORD_NULL_SEQUENCE_NUMBER;
    pl->t = DISCORD_GATEWAY_EVENT_NONE;

    return pl;
}

cJSON* discord_model_gateway_payload_to_cjson(discord_gateway_payload_t* payload) {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "op", payload->op);

    const char* d = "d";

    switch (payload->op) {
        case DISCORD_OP_HEARTBEAT:
            cJSON_AddItemToObject(root, d, discord_model_gateway_heartbeat_to_cjson((discord_gateway_heartbeat_t*) payload->d));
            break;

        case DISCORD_OP_IDENTIFY:
            cJSON_AddItemToObject(root, d, discord_model_gateway_identify_to_cjson((discord_gateway_identify_t*) payload->d));
            break;
        
        default:
            DISCORD_LOGW("Cannot recognize payload type");
            cJSON_Delete(root);
            root = NULL;
            break;
    }

    return root;
}

void discord_model_gateway_payload_free(discord_gateway_payload_t* payload) {
    if(payload == NULL)
        return;

    switch (payload->op) {
        case DISCORD_OP_HELLO:
            discord_model_gateway_hello_free((discord_gateway_hello_t*) payload->d);
            break;

        case DISCORD_OP_DISPATCH:
            discord_model_gateway_dispatch_event_data_free(payload);
            break;

        case DISCORD_OP_HEARTBEAT:
        case DISCORD_OP_HEARTBEAT_ACK:
            // Ignore
            break;

        case DISCORD_OP_IDENTIFY:
            discord_model_gateway_identify_free((discord_gateway_identify_t*) payload->d);
            break;
        
        default:
            DISCORD_LOGW("Cannot recognize payload type. Possible memory leak.");
            break;
    }

    free(payload);
}

discord_gateway_payload_t* discord_model_gateway_payload_deserialize(const char* json, size_t length) {
    cJSON* cjson = cJSON_ParseWithLength(json, length);
    
    if(cjson == NULL) {
        DISCORD_LOGW("JSON parsing (syntax?) error");
        return NULL;
    }

    discord_gateway_payload_t* pl = discord_model_gateway_payload(
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
            pl->d = discord_model_gateway_hello(cJSON_GetObjectItem(d, "heartbeat_interval")->valueint);
            break;

        case DISCORD_OP_DISPATCH:
            pl->t = discord_model_gateway_dispatch_event_name_map(cJSON_GetObjectItem(cjson, "t")->valuestring);
            pl->d = discord_model_gateway_dispatch_event_data_from_cjson(pl->t, d);
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

char* discord_model_gateway_payload_serialize(discord_gateway_payload_t* payload) {
    cJSON* cjson = discord_model_gateway_payload_to_cjson(payload);
    discord_model_gateway_payload_free(payload);

    char* payload_raw = cJSON_PrintUnformatted(cjson);
    cJSON_Delete(cjson);

    return payload_raw;
}

discord_gateway_event_t discord_model_gateway_dispatch_event_name_map(const char* name) {
    if(streq("READY", name)) {
        return DISCORD_GATEWAY_EVENT_READY;
    } else if(streq("MESSAGE_CREATE", name)) {
        return DISCORD_GATEWAY_EVENT_MESSAGE_CREATE;
    }

    return DISCORD_GATEWAY_EVENT_UNKNOWN;
}

discord_gateway_payload_data_t discord_model_gateway_dispatch_event_data_from_cjson(discord_gateway_event_t e, cJSON* cjson) {
    switch (e) {
        case DISCORD_GATEWAY_EVENT_READY:
            return discord_model_gateway_session_from_cjson(cjson);
        
        case DISCORD_GATEWAY_EVENT_MESSAGE_CREATE:
            return discord_model_message_from_cjson(cjson);
        
        default:
            DISCORD_LOGW("Cannot recognize event type");
            return NULL;
    }
}

void discord_model_gateway_dispatch_event_data_free(discord_gateway_payload_t* payload) {
    if(payload == NULL)
        return;

    switch (payload->t) {
        case DISCORD_GATEWAY_EVENT_READY:
            return discord_model_gateway_session_free((discord_gateway_session_t*) payload->d);
        
        case DISCORD_GATEWAY_EVENT_MESSAGE_CREATE:
            return discord_model_message_free((discord_message_t*) payload->d);
        
        default:
            DISCORD_LOGW("Cannot recognize event type");
            return;
    }
}

cJSON* discord_model_gateway_heartbeat_to_cjson(discord_gateway_heartbeat_t* heartbeat) {
    int hb = *((int*) (heartbeat));

    return hb == DISCORD_NULL_SEQUENCE_NUMBER ? cJSON_CreateNull() : cJSON_CreateNumber(hb);
}

discord_gateway_hello_t* discord_model_gateway_hello(int heartbeat_interval) {
    discord_gateway_hello_t* hello = calloc(1, sizeof(discord_gateway_hello_t));

    hello->heartbeat_interval = heartbeat_interval;

    return hello;
}

void discord_model_gateway_hello_free(discord_gateway_hello_t* hello) {
    if(hello == NULL)
        return;

    free(hello);
}

discord_gateway_identify_properties_t* discord_model_gateway_identify_properties(const char* $os, const char* $browser, const char* $device) {
    discord_gateway_identify_properties_t* props = calloc(1, sizeof(discord_gateway_identify_properties_t));

    props->$os = strdup($os);
    props->$browser = strdup($browser);
    props->$device = strdup($device);

    return props;
}

cJSON* discord_model_gateway_identify_properties_to_cjson(discord_gateway_identify_properties_t* properties) {
    cJSON* root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "$os", properties->$os);
    cJSON_AddStringToObject(root, "$browser", properties->$browser);
    cJSON_AddStringToObject(root, "$device", properties->$device);

    return root;
}

void discord_model_gateway_identify_properties_free(discord_gateway_identify_properties_t* properties) {
    if(properties == NULL)
        return;

    free(properties->$os);
    free(properties->$browser);
    free(properties->$device);
    free(properties);
}

discord_gateway_identify_t* discord_model_gateway_identify(const char* token, int intents, discord_gateway_identify_properties_t* properties) {
    discord_gateway_identify_t* identify = calloc(1, sizeof(discord_gateway_identify_t));

    identify->token = strdup(token);
    identify->intents = intents;
    identify->properties = properties;

    return identify;
}

cJSON* discord_model_gateway_identify_to_cjson(discord_gateway_identify_t* identify) {
    cJSON* root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "token", identify->token);
    cJSON_AddNumberToObject(root, "intents", identify->intents);
    cJSON_AddItemToObject(root, "properties", discord_model_gateway_identify_properties_to_cjson(identify->properties));

    return root;
}

void discord_model_gateway_identify_free(discord_gateway_identify_t* identify) {
    if(identify == NULL)
        return;

    free(identify->token);
    discord_model_gateway_identify_properties_free(identify->properties);
    free(identify);
}

discord_gateway_session_user_t* discord_model_gateway_session_user_from_cjson(cJSON* root) {
    discord_gateway_session_user_t* user = calloc(1, sizeof(discord_gateway_session_user_t));

    user->id = strdup(cJSON_GetObjectItem(root, "id")->valuestring);
    user->username = strdup(cJSON_GetObjectItem(root, "username")->valuestring);
    user->discriminator = strdup(cJSON_GetObjectItem(root, "discriminator")->valuestring);

    return user;
}

void discord_model_gateway_session_user_free(discord_gateway_session_user_t* user) {
    if(user == NULL)
        return;
    
    free(user->id);
    free(user->username);
    free(user->discriminator);
    free(user);
}

discord_gateway_session_t* discord_model_gateway_session_from_cjson(cJSON* root) {
    discord_gateway_session_t* id = calloc(1, sizeof(discord_gateway_session_t));

    id->session_id = strdup(cJSON_GetObjectItem(root, "session_id")->valuestring);
    id->user = discord_model_gateway_session_user_from_cjson(cJSON_GetObjectItem(root, "user"));

    return id;
}

void discord_model_gateway_session_free(discord_gateway_session_t* id) {
    if(id == NULL)
        return;

    discord_model_gateway_session_user_free(id->user);
    free(id->session_id);
    free(id);
}