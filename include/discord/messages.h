#ifndef _DISCORD_MESSAGE_H_
#define _DISCORD_MESSAGE_H_

#include "discord.h"
#include "discord/models/message.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DISCORD_EMOJI_THUMBSUP          "%F0%9F%91%8D"  /* 👍 */
#define DISCORD_EMOJI_THUMBSDOWN        "%F0%9F%91%8E"  /* 👎 */
#define DISCORD_EMOJI_WAVE              "%F0%9F%91%8B"  /* 👋 */
#define DISCORD_EMOJI_SMILE             "%F0%9F%98%84"  /* 😄 */
#define DISCORD_EMOJI_SMIRK             "%F0%9F%98%8F"  /* 😏 */
#define DISCORD_EMOJI_UNAMUSED          "%F0%9F%98%92"  /* 😒 */
#define DISCORD_EMOJI_SUNGLASSES        "%F0%9F%98%8E"  /* 😎 */
#define DISCORD_EMOJI_CONFUSED          "%F0%9F%98%95"  /* 😕 */
#define DISCORD_EMOJI_WHITE_CHECK_MARK  "%E2%9C%85"     /* ✅ */
#define DISCORD_EMOJI_X                 "%E2%9D%8C"     /* ❌ */

discord_message_t* discord_message_send_(discord_client_handle_t client, discord_message_t* message, esp_err_t* err);
esp_err_t discord_message_send(discord_client_handle_t client, discord_message_t* message);
esp_err_t discord_message_react(discord_client_handle_t client, discord_message_t* message, const char* emoji);

#ifdef __cplusplus
}
#endif

#endif