#ifndef _DISCORD_MESSAGE_H_
#define _DISCORD_MESSAGE_H_

#include "discord.h"
#include "discord/models/message.h"
#include "discord/models/message_reaction.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DISCORD_EMOJI_THUMBSUP          "👍"
#define DISCORD_EMOJI_THUMBSDOWN        "👎"
#define DISCORD_EMOJI_WAVE              "👋"
#define DISCORD_EMOJI_SMILE             "😄"
#define DISCORD_EMOJI_SMIRK             "😏"
#define DISCORD_EMOJI_UNAMUSED          "😒"
#define DISCORD_EMOJI_SUNGLASSES        "😎"
#define DISCORD_EMOJI_CONFUSED          "😕"
#define DISCORD_EMOJI_WHITE_CHECK_MARK  "✅"
#define DISCORD_EMOJI_X                 "❌"

discord_message_t* discord_message_send_(discord_client_handle_t client, discord_message_t* message, esp_err_t* err);
esp_err_t discord_message_send(discord_client_handle_t client, discord_message_t* message);
esp_err_t discord_message_react(discord_client_handle_t client, discord_message_t* message, const char* emoji);

#ifdef __cplusplus
}
#endif

#endif