#include "esp_heap_caps.h"
#include "cJSON.h"
#include "esp_log.h"
#include "discord/private/_discord.h"
#include "discord/private/_models.h"
#include "discord/utils.h"

DISCORD_LOG_DEFINE_BASE();

discord_payload_t* discord_payload_ctor(int op, discord_payload_data_t d) {
    discord_payload_t* pl = calloc(1, sizeof(discord_payload_t));

    pl->op = op;
    pl->d = d;
    pl->s = DISCORD_NULL_SEQUENCE_NUMBER;
    pl->t = DISCORD_EVENT_NONE;

    return pl;
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

discord_identify_properties_t* discord_identify_properties_ctor_(const char* $os, const char* $browser, const char* $device) {
    discord_identify_properties_t* props = calloc(1, sizeof(discord_identify_properties_t));

    props->$os = strdup($os);
    props->$browser = strdup($browser);
    props->$device = strdup($device);

    return props;
}

void discord_identify_properties_free(discord_identify_properties_t* properties) {
    if(!properties)
        return;

    free(properties->$os);
    free(properties->$browser);
    free(properties->$device);
    free(properties);
}

discord_identify_t* discord_identify_ctor_(const char* token, int intents, discord_identify_properties_t* properties) {
    discord_identify_t* identify = calloc(1, sizeof(discord_identify_t));

    identify->token = strdup(token);
    identify->intents = intents;
    identify->properties = properties;

    return identify;
}

void discord_identify_free(discord_identify_t* identify) {
    if(!identify)
        return;

    free(identify->token);
    discord_identify_properties_free(identify->properties);
    free(identify);
}