#pragma once

#include "esp_system.h"
#include "discord.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct discord_gateway* discord_gateway_handle_t;

typedef struct {
    discord_bot_config_t bot;
} discord_gateway_config_t;

discord_gateway_handle_t discord_gw_init(const discord_gateway_config_t* config);
esp_err_t discord_gw_open(discord_gateway_handle_t gateway);
esp_err_t discord_gw_destroy(discord_gateway_handle_t gateway);

#ifdef __cplusplus
}
#endif