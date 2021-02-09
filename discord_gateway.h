#pragma once

#include "esp_system.h"
#include "discord.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t discord_gw_open(const discord_bot_config_t* config);

#ifdef __cplusplus
}
#endif