#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "discord.h"
#include "_discord.h"
#include "_models.h"

enum {
    DISCORD_OP_DISPATCH,                    /*!< [Receive] An event was dispatched */
    DISCORD_OP_HEARTBEAT,                   /*!< [Send/Receive] An event was dispatched */
    DISCORD_OP_IDENTIFY,                    /*!< [Send] Starts a new session during the initial handshake */
    DISCORD_OP_PRESENCE_UPDATE,             /*!< [Send] Update the client's presence */
    DISCORD_OP_VOICE_STATE_UPDATE,          /*!< [Send] Used to join/leave or move between voice channels */
    DISCORD_OP_RESUME = 6,                  /*!< [Send] Resume a previous session that was disconnected */
    DISCORD_OP_RECONNECT,                   /*!< [Receive] You should attempt to reconnect and resume immediately */
    DISCORD_OP_REQUEST_GUILD_MEMBERS,       /*!< [Send] Request information about offline guild members in a large guild */
    DISCORD_OP_INVALID_SESSION,             /*!< [Receive] The session has been invalidated. You should reconnect and identify/resume accordingly */
    DISCORD_OP_HELLO,                       /*!< [Receive] Sent immediately after connecting, contains the heartbeat_interval to use */
    DISCORD_OP_HEARTBEAT_ACK,               /*!< [Receive] Sent in response to receiving a heartbeat to acknowledge that it has been received */
};

#define _DISCORD_CLOSEOP_MIN DISCORD_CLOSEOP_UNKNOWN_ERROR
#define _DISCORD_CLOSEOP_MAX DISCORD_CLOSEOP_DISALLOWED_INTENTS

esp_err_t gw_init(discord_client_handle_t client);
esp_err_t gw_reset(discord_client_handle_t client);
esp_err_t gw_send(discord_client_handle_t client, discord_gateway_payload_t* payload);
esp_err_t gw_open(discord_client_handle_t client);
bool gw_is_open(discord_client_handle_t client);
esp_err_t gw_start(discord_client_handle_t client);
esp_err_t gw_close(discord_client_handle_t client, discord_close_reason_t reason);
discord_gateway_close_code_t gw_close_opcode(discord_client_handle_t client);
char* gw_close_desc(discord_client_handle_t client);
#define gw_heartbeat_init(client) gw_heartbeat_stop(client)
esp_err_t gw_heartbeat_start(discord_client_handle_t client, discord_gateway_hello_t* hello);
esp_err_t gw_heartbeat_send_if_expired(discord_client_handle_t client);
esp_err_t gw_heartbeat_stop(discord_client_handle_t client);
esp_err_t gw_buffer_websocket_data(discord_client_handle_t client, esp_websocket_event_data_t* data);
esp_err_t gw_handle_buffered_data(discord_client_handle_t client);
void gw_websocket_event_handler(void* handler_arg, esp_event_base_t base, int32_t event_id, void* event_data);
esp_err_t gw_identify(discord_client_handle_t client);
esp_err_t gw_dispatch(discord_client_handle_t client, discord_gateway_payload_t* payload);

#ifdef __cplusplus
}
#endif