#ifndef _DISCORD_OTA_H_
#define _DISCORD_OTA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "estr.h"
#include "discord.h"
#include "discord/message.h"

typedef struct {
    bool success_feedback_disabled;
    bool error_feedback_disabled;
    bool administrator_only_disabled;
} discord_ota_config_t;

esp_err_t discord_ota(discord_handle_t handle, discord_message_t* firmware_message, discord_ota_config_t* config);
esp_err_t discord_ota_keep(bool keep_or_rollback);

#ifdef __cplusplus
}
#endif

#endif