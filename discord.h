#pragma once

#include "esp_system.h"
#include "discord_models.h"

#ifdef __cplusplus
extern "C" {
#endif

#define streq(s1, s2) strcmp(s1, s2) == 0

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

enum {
    DISCORD_OP_DISPATCH,               /*!< [Receive] An event was dispatched */
    DISCORD_OP_HEARTBEAT,              /*!< [Send/Receive] An event was dispatched */
    DISCORD_OP_IDENTIFY,               /*!< [Send] Starts a new session during the initial handshake */
    DISCORD_OP_PRESENCE_UPDATE,        /*!< [Send] Update the client's presence */
    DISCORD_OP_VOICE_STATE_UPDATE,     /*!< [Send] Used to join/leave or move between voice channels */
    DISCORD_OP_RESUME = 6,             /*!< [Send] Resume a previous session that was disconnected */
    DISCORD_OP_RECONNECT,              /*!< [Receive] You should attempt to reconnect and resume immediately */
    DISCORD_OP_REQUEST_GUILD_MEMBERS,  /*!< [Send] Request information about offline guild members in a large guild */
    DISCORD_OP_INVALID_SESSION,        /*!< [Receive] The session has been invalidated. You should reconnect and identify/resume accordingly */
    DISCORD_OP_HELLO,                  /*!< [Receive] Sent immediately after connecting, contains the heartbeat_interval to use */
    DISCORD_OP_HEARTBEAT_ACK,          /*!< [Receive] Sent in response to receiving a heartbeat to acknowledge that it has been received */
};

typedef struct discord_client* discord_client_handle_t;

typedef struct {
    char* token;
    int intents;
} discord_client_config_t;

discord_client_handle_t discord_init(const discord_client_config_t* config);
esp_err_t discord_login(discord_client_handle_t client);
esp_err_t discord_destroy(discord_client_handle_t client);

#ifdef __cplusplus
}
#endif