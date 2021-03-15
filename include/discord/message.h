#ifndef _DISCORD_MESSAGE_H_
#define _DISCORD_MESSAGE_H_

#include "discord.h"
#include "discord/user.h"
#include "discord/member.h"
#include "discord/message_reaction.h"
#include "discord/attachment.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DISCORD_MESSAGE_UNDEFINED = -1,
    DISCORD_MESSAGE_DEFAULT,
    DISCORD_MESSAGE_RECIPIENT_ADD,
    DISCORD_MESSAGE_RECIPIENT_REMOVE,
    DISCORD_MESSAGE_CALL,
    DISCORD_MESSAGE_CHANNEL_NAME_CHANGE,
    DISCORD_MESSAGE_CHANNEL_ICON_CHANGE,
    DISCORD_MESSAGE_CHANNEL_PINNED_MESSAGE,
    DISCORD_MESSAGE_GUILD_MEMBER_JOIN,
    DISCORD_MESSAGE_USER_PREMIUM_GUILD_SUBSCRIPTION,
    DISCORD_MESSAGE_USER_PREMIUM_GUILD_SUBSCRIPTION_TIER_1,
    DISCORD_MESSAGE_USER_PREMIUM_GUILD_SUBSCRIPTION_TIER_2,
    DISCORD_MESSAGE_USER_PREMIUM_GUILD_SUBSCRIPTION_TIER_3,
    DISCORD_MESSAGE_CHANNEL_FOLLOW_ADD,
    DISCORD_MESSAGE_GUILD_DISCOVERY_DISQUALIFIED = 14,
    DISCORD_MESSAGE_GUILD_DISCOVERY_REQUALIFIED,
    DISCORD_MESSAGE_REPLY = 19,
    DISCORD_MESSAGE_APPLICATION_COMMAND
} discord_message_type_t;

typedef struct {
    char* id;
    discord_message_type_t type;
    char* content;
    char* channel_id;
    discord_user_t* author;
    char* guild_id;
    discord_member_t* member;
    discord_attachment_t** attachments;
    uint8_t _attachments_len;
} discord_message_t;

esp_err_t discord_message_send(discord_handle_t client, discord_message_t* message, discord_message_t** out_result);
esp_err_t discord_message_react(discord_handle_t client, discord_message_t* message, const char* emoji);
esp_err_t discord_message_download_attachment(discord_handle_t client, discord_message_t* message, uint8_t attachment_index, discord_download_handler_t download_handler);
void discord_message_free(discord_message_t* message);

#ifdef __cplusplus
}
#endif

#endif