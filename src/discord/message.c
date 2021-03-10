#include "discord/message.h"
#include "discord/private/_discord.h"
#include "discord/private/_api.h"
#include "discord/private/_json.h"
#include "cutils.h"
#include "estr.h"

DISCORD_LOG_DEFINE_BASE();

static discord_message_t* _discord_message_send_(discord_handle_t client, discord_message_t* message, esp_err_t* err, bool _return) {
    if(!client || !message || !message->content || !message->channel_id) {
        DISCORD_LOGE("Invalid args");
        *err = ESP_ERR_INVALID_ARG;
        return NULL;
    }

    discord_api_response_t* res = dcapi_post(
        client,
        estr_cat("/channels/", message->channel_id, "/messages"),
        discord_json_serialize(message),
        _return
    );

    *err = dcapi_response_to_esp_err(res);

    discord_message_t* sent_message = NULL;

    if(_return && *err == ESP_OK) {
        if(res->data_len <= 0) {
            DISCORD_LOGW("Message sent but cannot return");
        } else {
            sent_message = discord_json_deserialize_(message, res->data, res->data_len);
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

    char* _emoji = estr_url_encode(emoji);
    esp_err_t err = dcapi_put_(client, estr_cat("/channels/", message->channel_id, "/messages/", message->id, "/reactions/", _emoji, "/@me"), NULL);
    free(_emoji);

    return err;
}

static size_t _downloading_attachment_size;

static bool dc_message_attachment_download_approver(size_t content_length) {
    return _downloading_attachment_size == content_length;
}

esp_err_t discord_message_download_attachment(discord_handle_t client, discord_message_t* message, uint8_t attachment_index, discord_download_handler_t download_handler) {
    if(!client || !message || !message->attachments) {
        DISCORD_LOGE("Invalid args");
        return ESP_ERR_INVALID_ARG;
    }

    if(message->_attachments_len <= attachment_index) {
        DISCORD_LOGE("Message does not contain attachment with index %d", attachment_index);
        return ESP_ERR_INVALID_ARG;
    }

    discord_attachment_t* attach = message->attachments[attachment_index];
    _downloading_attachment_size = attach->size;
    discord_api_response_t* res = dcapi_download_(client, attach->url, &dc_message_attachment_download_approver, download_handler);
    esp_err_t err = dcapi_response_to_esp_err(res);
    dcapi_response_free(client, res);

    return err;
}

void discord_message_free(discord_message_t* message) {
    if(!message)
        return;
    
    free(message->id);
    free(message->content);
    free(message->channel_id);
    discord_user_free(message->author);
    free(message->guild_id);
    discord_member_free(message->member);
    cu_list_freex(message->attachments, message->_attachments_len, discord_attachment_free);
    free(message);
}