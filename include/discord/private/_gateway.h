#ifndef _DISCORD_PRIVATE_GATEWAY_H_
#define _DISCORD_PRIVATE_GATEWAY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "discord.h"
#include "_discord.h"
#include "_models.h"

esp_err_t dcgw_init(discord_handle_t client);
/**
 * @brief Send payload (serialized to json) to gateway. Payload will be automatically freed
 */
esp_err_t dcgw_send(discord_handle_t client, discord_payload_t* payload);
bool dcgw_is_open(discord_handle_t client);
esp_err_t dcgw_open(discord_handle_t client);
esp_err_t dcgw_start(discord_handle_t client);
esp_err_t dcgw_close(discord_handle_t client, discord_gateway_close_reason_t reason);
esp_err_t dcgw_get_close_desc(discord_handle_t client, char** out_description);
esp_err_t dcgw_destroy(discord_handle_t client);
esp_err_t dcgw_queue_flush(discord_handle_t client);
esp_err_t dcgw_heartbeat_send_if_expired(discord_handle_t client);
esp_err_t dcgw_handle_payload(discord_handle_t client, discord_payload_t* payload);

#ifdef __cplusplus
}
#endif

#endif