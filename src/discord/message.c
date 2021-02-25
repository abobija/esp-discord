#include "discord/message.h"
#include "discord/private/_discord.h"
#include "discord/private/_api.h"
#include "discord/private/_json.h"
#include "discord/utils.h"

DISCORD_LOG_DEFINE_BASE();

discord_message_t* discord_message_ctor(char* id, char* content, char* channel_id, discord_user_t* author, char* guild_id, discord_member_t* member, discord_attachment_t** attachments, uint8_t attachments_len) {
    discord_message_t* message = calloc(1, sizeof(discord_message_t));

    message->id = id;
    message->content = content;
    message->channel_id = channel_id;
    message->author = author;
    message->guild_id = guild_id;
    message->member = member;
    message->attachments = attachments;
    message->_attachments_len = attachments_len;

    return message;
}

static discord_message_t* _discord_message_send_(discord_handle_t client, discord_message_t* message, esp_err_t* err, bool _return) {
    if(!client || !message || !message->content || !message->channel_id) {
        DISCORD_LOGE("Invalid args");
        *err = ESP_ERR_INVALID_ARG;
        return NULL;
    }

    discord_api_response_t* res = dcapi_post(
        client,
        discord_strcat("/channels/", message->channel_id, "/messages"),
        discord_message_serialize(message),
        _return
    );

    *err = dcapi_response_to_esp_err(res);

    discord_message_t* sent_message = NULL;

    if(_return && *err == ESP_OK) {
        if(res->data_len <= 0) {
            DISCORD_LOGW("Message sent but cannot return");
        } else {
            sent_message = discord_message_deserialize(res->data, res->data_len);
        }
    }
    
    dcapi_response_free(client, res);

    return sent_message;
}

discord_message_t* discord_message_send_(discord_handle_t client, discord_message_t* message, esp_err_t* err) {
    esp_err_t _err;
    discord_message_t* sent_message = _discord_message_send_(client, message, &_err, true);
    if(err) *err = _err;

    return sent_message;
}

esp_err_t discord_message_send(discord_handle_t client, discord_message_t* message) {
    esp_err_t err;
    _discord_message_send_(client, message, &err, false);

    return err;
}

esp_err_t discord_message_react(discord_handle_t client, discord_message_t* message, const char* emoji) {
    if(!client || !message || !message->id || !message->channel_id) {
        DISCORD_LOGE("Invalid args");
        return ESP_FAIL;
    }

    char* _emoji = discord_url_encode(emoji);
    esp_err_t err = dcapi_put_(client, discord_strcat("/channels/", message->channel_id, "/messages/", message->id, "/reactions/", _emoji, "/@me"), NULL);
    free(_emoji);

    return err;
}

void discord_message_free(discord_message_t* message) {
    if(message == NULL)
        return;

    free(message->id);
    free(message->content);
    free(message->channel_id);
    discord_user_free(message->author);
    free(message->guild_id);
    discord_member_free(message->member);
    if(message->attachments) {
        for(uint8_t i = 0; i < message->_attachments_len; i++) {
            discord_attachment_free(message->attachments[i]);
        }
        free(message->attachments);
    }
    free(message);
}