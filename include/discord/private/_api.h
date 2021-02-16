#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "discord.h"
#include "_discord.h"
#include "_models.h"
#include "discord/models.h"

esp_err_t dcapi_init(discord_client_handle_t client);
esp_err_t dcapi_destroy(discord_client_handle_t client);
esp_err_t dcapi_send_message(discord_client_handle_t client, discord_message_t* msg);

#ifdef __cplusplus
}
#endif