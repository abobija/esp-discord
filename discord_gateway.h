#pragma once

#include "esp_system.h"
#include "discord.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t discord_gw_init(const discord_bot_config_t config);
esp_err_t discord_gw_open();

#ifdef __cplusplus
}
#endif