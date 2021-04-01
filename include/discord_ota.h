#ifndef _DISCORD_OTA_H_
#define _DISCORD_OTA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "estr.h"
#include "discord.h"
#include "discord/message.h"

#define DISCORD_OTA_DEFAULT_PREFIX "!ota"

typedef struct discord_ota* discord_ota_handle_t;

typedef struct {
    char* prefix;                       /*<! Prefix with which message needs to start in order to perform OTA update */
    bool multiple_ota;                  /*<! Set this to true if there is need to perform OTA update on multiple devices at once */
    bool success_feedback_disabled;     /*<! Disable sending feedback on failure */
    bool error_feedback_disabled;       /*<! Disable sending feedback on success */
    bool administrator_only_disabled;   /*<! Disable option that only Administrators can perform OTA update */
    discord_channel_t* channel;         /*<! Channel in which OTA update can be performed. Id or Name can be provided. Id has higher priority over the channel Name (if both are provided). Set to NULL to allow all channels. Note: Maybe you will need to increase the size of Api buffer for using this option with providing channel Name instead of Id */
} discord_ota_config_t;

/**
 * @brief Initialize discord OTA ability
 * @param client Discord bot handle
 * @param config OTA config. Provide NULL for default configuration
 * @return ESP_OK on success
 */
esp_err_t discord_ota_init(discord_handle_t client, discord_ota_config_t* config);

/**
 * @brief Keep or reject new firmware
 * @param keep_or_rollback Set to false to rollback to previous firmware version
 * @return ESP_OK on success
 */
esp_err_t discord_ota_keep(bool keep_or_rollback);

/**
 * @brief Free the resources occupied by OTA
 * @param client Discord bot handle
 */
void discord_ota_destroy(discord_handle_t client);

#ifdef __cplusplus
}
#endif

#endif