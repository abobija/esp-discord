#ifndef _DISCORD_MESSAGES_H_
#define _DISCORD_MESSAGES_H_

#include "discord.h"
#include "discord/models/message.h"
#include "discord/models/message_reaction.h"

#ifdef __cplusplus
extern "C" {
#endif

discord_message_t* discord_message_send_(discord_handle_t client, discord_message_t* message, esp_err_t* err);
esp_err_t discord_message_send(discord_handle_t client, discord_message_t* message);
esp_err_t discord_message_react(discord_handle_t client, discord_message_t* message, const char* emoji);

#ifdef __cplusplus
}
#endif

#endif