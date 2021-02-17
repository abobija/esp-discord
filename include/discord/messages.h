#ifndef _DISCORD_MESSAGE_H_
#define _DISCORD_MESSAGE_H_

#include "discord.h"
#include "discord/models/message.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t discord_message_send(discord_client_handle_t client, discord_message_t* message);

#ifdef __cplusplus
}
#endif

#endif