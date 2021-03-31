#ifndef _DISCORD_OTA_H_
#define _DISCORD_OTA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "estr.h"
#include "discord.h"
#include "discord/message.h"

#define DISCORD_OTA_DEFAULT_PREFIX "!ota"

typedef struct {
    char* prefix;                       /*<! Prefix with which message needs to start in order to perform OTA update */
    bool success_feedback_disabled;     /*<! Disable sending feedback on failure */
    bool error_feedback_disabled;       /*<! Disable sending feedback on success */
    bool administrator_only_disabled;   /*<! Disable option that only Administrators can perform OTA update */
    discord_channel_t* channel;         /*<! Channel in which OTA update can be performed. Id of channel has higher priority over the channel Name. Set to NULL to allow all channels. Note: Maybe you will need to increase the size of Api buffer for using this option with providing channel Name instead of Id */
} discord_ota_config_t;

/**
 * @brief Performs Discord OTA update
 * @param handle Discord bot handle
 * @param firmware_message Message that contains new firmware as a attachment
 * @param config Configuration for OTA update. Provide NULL for default configurations
 * @return ESP_OK on success
 */
esp_err_t discord_ota(discord_handle_t handle, discord_message_t* firmware_message, discord_ota_config_t* config);

/**
 * @brief Keep or reject new firmware
 * @param keep_or_rollback Set to false to rollback to previous firmware version
 * @return ESP_OK on success
 */
esp_err_t discord_ota_keep(bool keep_or_rollback);

#ifdef __cplusplus
}
#endif

#endif