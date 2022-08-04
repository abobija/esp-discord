#include "esp_heap_caps.h"
#include "cJSON.h"
#include "esp_log.h"
#include "discord/private/_discord.h"
#include "discord/private/_models.h"

#include "discord/session.h"
#include "discord/user.h"
#include "discord/member.h"
#include "discord/message.h"
#include "discord/message_reaction.h"
#include "discord/role.h"
#include "discord/voice_state.h"

DISCORD_LOG_DEFINE_BASE();

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
        
        case DISCORD_EVENT_VOICE_STATE_UPDATED:
            return discord_voice_state_free((discord_voice_state_t*) payload->d);

        default:
            DISCORD_LOGW("Cannot recognize event type");
            return;
    }
}

void discord_hello_free(discord_hello_t* hello) {
    if(!hello)
        return;

    free(hello);
}

void discord_identify_properties_free(discord_identify_properties_t* properties) {
    if(!properties)
        return;

    free(properties->os);
    free(properties->browser);
    free(properties->device);
    free(properties);
}

void discord_identify_free(discord_identify_t* identify) {
    if(!identify)
        return;

    free(identify->token);
    discord_identify_properties_free(identify->properties);
    free(identify);
}