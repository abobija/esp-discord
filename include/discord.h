#pragma once

#include "esp_err.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DISCORD_INTENT_GUILDS                    (1 << 0)
#define DISCORD_INTENT_GUILD_MEMBERS             (1 << 1)
#define DISCORD_INTENT_GUILD_BANS                (1 << 2)
#define DISCORD_INTENT_GUILD_EMOJIS              (1 << 3)
#define DISCORD_INTENT_GUILD_INTEGRATIONS        (1 << 4)
#define DISCORD_INTENT_GUILD_WEBHOOKS            (1 << 5)
#define DISCORD_INTENT_GUILD_INVITES             (1 << 6)
#define DISCORD_INTENT_GUILD_VOICE_STATES        (1 << 7)
#define DISCORD_INTENT_GUILD_PRESENCES           (1 << 8)
#define DISCORD_INTENT_GUILD_MESSAGES            (1 << 9)
#define DISCORD_INTENT_GUILD_MESSAGE_REACTIONS   (1 << 10)
#define DISCORD_INTENT_GUILD_MESSAGE_TYPING      (1 << 11)
#define DISCORD_INTENT_DIRECT_MESSAGES           (1 << 12)
#define DISCORD_INTENT_DIRECT_MESSAGE_REACTIONS  (1 << 13)
#define DISCORD_INTENT_DIRECT_MESSAGE_TYPING     (1 << 14)

typedef struct {
    char* token;
    int intents;
    int buffer_size;
} discord_client_config_t;

typedef struct discord_client* discord_client_handle_t;

ESP_EVENT_DECLARE_BASE(DISCORD_EVENTS);

typedef enum {
    DISCORD_EVENT_ANY = ESP_EVENT_ANY_ID,
    DISCORD_EVENT_CONNECTED,
    DISCORD_EVENT_MESSAGE_RECEIVED
} discord_event_id_t;

typedef void* discord_event_data_ptr_t;

typedef struct {
    discord_client_handle_t client;
    discord_event_data_ptr_t ptr;
} discord_event_data_t;

discord_client_handle_t discord_create(const discord_client_config_t* config);
esp_err_t discord_login(discord_client_handle_t client);
esp_err_t discord_register_events(discord_client_handle_t client, discord_event_id_t event, esp_event_handler_t event_handler, void* event_handler_arg);
esp_err_t discord_logout(discord_client_handle_t client);
esp_err_t discord_destroy(discord_client_handle_t client);

esp_err_t discord_send_message(discord_client_handle_t client, char* content, char* channel_id);

#ifdef __cplusplus
}
#endif