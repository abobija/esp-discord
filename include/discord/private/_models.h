#ifndef _DISCORD_PRIVATE_MODELS_H_
#define _DISCORD_PRIVATE_MODELS_H_

#include "cJSON.h"
#include "discord.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DISCORD_NULL_SEQUENCE_NUMBER 0

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
    char* os;
    char* browser;
    char* device;
} discord_identify_properties_t;

typedef struct {
    char* token;
    int intents;
    discord_identify_properties_t* properties;
} discord_identify_t;

void discord_payload_free(discord_payload_t* payload);

void discord_dispatch_event_data_free(discord_payload_t* payload);

void discord_hello_free(discord_hello_t* hello);

void discord_identify_properties_free(discord_identify_properties_t* properties);

void discord_identify_free(discord_identify_t* identify);

#ifdef __cplusplus
}
#endif

#endif