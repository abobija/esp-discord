#ifndef _DISCORD_PRIVATE_GATEWAY_H_
#define _DISCORD_PRIVATE_GATEWAY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "discord.h"
#include "_discord.h"
#include "_models.h"

esp_err_t dcgw_init(discord_client_handle_t client);
esp_err_t dcgw_send(discord_client_handle_t client, discord_gateway_payload_t* payload);
esp_err_t dcgw_open(discord_client_handle_t client);
esp_err_t dcgw_start(discord_client_handle_t client);
esp_err_t dcgw_close(discord_client_handle_t client, discord_close_reason_t reason);
char* dcgw_close_desc(discord_client_handle_t client);
esp_err_t dcgw_heartbeat_send_if_expired(discord_client_handle_t client);
esp_err_t dcgw_handle_buffered_data(discord_client_handle_t client);

#ifdef __cplusplus
}
#endif

#endif