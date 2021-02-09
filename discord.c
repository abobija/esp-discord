#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_websocket_client.h"
#include "discord.h"
#include "discord_gateway.h"

static const char* TAG = "discord";

static const discord_bot_config_t* _config;

esp_err_t discord_init(const discord_bot_config_t* config) {
    _config = config;
    return ESP_OK;
}

esp_err_t discord_login() {
    return discord_gw_open(&_config);
}