#pragma once

#include "esp_system.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char* token;
} discord_bot_config_t;

esp_err_t discord_init(const discord_bot_config_t* config);
esp_err_t discord_login();

#ifdef __cplusplus
}
#endif