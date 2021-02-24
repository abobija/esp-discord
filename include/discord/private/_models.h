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
void discord_payload_free(discord_payload_t* payload);

void discord_dispatch_event_data_free(discord_payload_t* payload);

discord_hello_t* discord_hello_ctor(int heartbeat_interval);
void discord_hello_free(discord_hello_t* hello);

discord_identify_properties_t* discord_identify_properties_ctor_(const char* $os, const char* $browser, const char* $device);
void discord_identify_properties_free(discord_identify_properties_t* properties);

discord_identify_t* discord_identify_ctor_(const char* token, int intents, discord_identify_properties_t* properties);
void discord_identify_free(discord_identify_t* identify);

#ifdef __cplusplus
}
#endif

#endif