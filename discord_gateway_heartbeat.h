#pragma once

#include "esp_system.h"
#include "esp_websocket_client.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t discord_gw_heartbeat_init(esp_websocket_client_handle_t ws_client);
esp_err_t discord_gw_heartbeat_start(int interval);

#ifdef __cplusplus
}
#endif