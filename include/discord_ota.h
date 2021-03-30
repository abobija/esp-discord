#ifndef _DISCORD_OTA_H_
#define _DISCORD_OTA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "estr.h"
#include "discord.h"
#include "discord/message.h"

esp_err_t discord_ota(discord_handle_t handle, discord_message_t* firmware_message);

#ifdef __cplusplus
}
#endif

#endif