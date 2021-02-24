#ifndef _DISCORD_MESSAGE_H_
#define _DISCORD_MESSAGE_H_

#include "discord.h"
#include "discord/user.h"
#include "discord/member.h"
#include "discord/message_reaction.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char* id;
    char* content;
    char* channel_id;
    discord_user_t* author;
    char* guild_id;
    discord_member_t* member;
} discord_message_t;

discord_message_t* discord_message_send_(discord_handle_t client, discord_message_t* message, esp_err_t* err);
esp_err_t discord_message_send(discord_handle_t client, discord_message_t* message);
esp_err_t discord_message_react(discord_handle_t client, discord_message_t* message, const char* emoji);
void discord_message_free(discord_message_t* message);

#ifdef __cplusplus
}
#endif

#endif