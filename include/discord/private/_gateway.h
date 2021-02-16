#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "discord.h"
#include "_discord.h"
#include "_models.h"

esp_err_t gw_init(discord_client_handle_t client);
esp_err_t gw_reset(discord_client_handle_t client);
esp_err_t gw_send(discord_client_handle_t client, discord_gateway_payload_t* payload);
esp_err_t gw_open(discord_client_handle_t client);
bool gw_is_open(discord_client_handle_t client);
esp_err_t gw_start(discord_client_handle_t client);
esp_err_t gw_close(discord_client_handle_t client, discord_close_reason_t reason);
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