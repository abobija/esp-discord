#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_websocket_client.h"
#include "discord.h"
#include "discord_gateway.h"

discord_gateway_handle_t gateway;

esp_err_t discord_init(const discord_bot_config_t config) {
    discord_gateway_config_t gateway_cfg = {
        .bot = config
    };

    gateway = discord_gw_init(&gateway_cfg);
    
    return ESP_OK;
}

esp_err_t discord_login() {
    return discord_gw_open(gateway);
}