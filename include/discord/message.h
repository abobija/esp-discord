#ifndef _DISCORD_MESSAGE_H_
#define _DISCORD_MESSAGE_H_

#include "discord.h"
#include "discord/user.h"
#include "discord/member.h"
#include "discord/message_reaction.h"
#include "discord/attachment.h"
#include "discord/embed.h"

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
    DISCORD_MESSAGE_USER_JOIN,
    DISCORD_MESSAGE_GUILD_BOOST,
    DISCORD_MESSAGE_GUILD_BOOST_TIER_1,
    DISCORD_MESSAGE_GUILD_BOOST_TIER_2,
    DISCORD_MESSAGE_GUILD_BOOST_TIER_3,
    DISCORD_MESSAGE_CHANNEL_FOLLOW_ADD,
    DISCORD_MESSAGE_GUILD_DISCOVERY_DISQUALIFIED = 14,
    DISCORD_MESSAGE_GUILD_DISCOVERY_REQUALIFIED,
    DISCORD_MESSAGE_GUILD_DISCOVERY_GRACE_PERIOD_INITIAL_WARNING,
    DISCORD_MESSAGE_GUILD_DISCOVERY_GRACE_PERIOD_FINAL_WARNING,
    DISCORD_MESSAGE_THREAD_CREATED,
    DISCORD_MESSAGE_REPLY,
    DISCORD_MESSAGE_CHAT_INPUT_COMMAND,
    DISCORD_MESSAGE_THREAD_STARTER_MESSAGE,
    DISCORD_MESSAGE_GUILD_INVITE_REMINDER,
    DISCORD_MESSAGE_CONTEXT_MENU_COMMAND,
    DISCORD_MESSAGE_AUTO_MODERATION_ACTION,
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
    discord_embed_t** embeds;
    uint8_t _embeds_len;
} discord_message_t;

typedef enum {
    DISCORD_MESSAGE_WORD_DEFAULT,               /*<! Regular text */
    DISCORD_MESSAGE_WORD_USER,                  /*<! User mention by username */
    DISCORD_MESSAGE_WORD_USER_NICKNAME,         /*<! User mention by nickname */
    DISCORD_MESSAGE_WORD_CHANNEL,               /*<! Channel mention */
    DISCORD_MESSAGE_WORD_ROLE,                  /*<! Role mention */
    DISCORD_MESSAGE_WORD_CUSTOM_EMOJI,          /*<! Custom emoji */
    DISCORD_MESSAGE_WORD_CUSTOM_EMOJI_ANIMATED  /*<! Custom animated emoji */
} discord_message_word_type_t;

typedef struct {
    discord_message_word_type_t type;
	const char* name;
	size_t name_len;
	const char* id;
    size_t id_len;
} discord_message_word_t;

#define discord_message_dump_log(LOG_FOO, TAG, msg) \
    LOG_FOO(TAG, "New message (content=%s, autor=%s#%s, bot=%s, attachments_len=%d, channel=%s, dm=%s, guild=%s)", \
        msg->content, \
        msg->author->username, \
        msg->author->discriminator, \
        msg->author->bot ? "true" : "false", \
        msg->_attachments_len, \
        msg->channel_id, \
        msg->guild_id ? "false" : "true", \
        msg->guild_id ? msg->guild_id : "NULL" \
    );

esp_err_t discord_message_send(discord_handle_t client, discord_message_t* message, discord_message_t** out_result);
esp_err_t discord_message_react(discord_handle_t client, discord_message_t* message, const char* emoji);
esp_err_t discord_message_download_attachment(discord_handle_t client, discord_message_t* message, uint8_t attachment_index, discord_download_handler_t download_handler, void* arg);
esp_err_t discord_message_word_parse(const char* word, discord_message_word_t** out_word);
esp_err_t discord_message_add_attachment(discord_message_t* message, discord_attachment_t* attachment);
esp_err_t discord_message_add_embed(discord_message_t* message, discord_embed_t* embed);
void discord_message_free(discord_message_t* message);

#ifdef __cplusplus
}
#endif

#endif