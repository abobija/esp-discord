#include "discord/messages.h"
#include "discord/private/_discord.h"
#include "discord/private/_api.h"

DISCORD_LOG_DEFINE_BASE();

static discord_message_t* _discord_message_send_(discord_client_handle_t client, discord_message_t* message, esp_err_t* err, bool _return) {
    if(message == NULL) {
        *err = ESP_FAIL;
        return NULL;
    }

    if(message->content == NULL) {
        DISCORD_LOGE("Missing content");
        *err = ESP_ERR_INVALID_ARG;
        return NULL;
    }

    if(message->channel_id == NULL) {
        DISCORD_LOGE("Missing channel_id");
        *err = ESP_ERR_INVALID_ARG;
        return NULL;
    }

    discord_api_response_t* res = dcapi_post(
        client,
        STRCAT("/channels/", message->channel_id, "/messages"),
        discord_model_message_serialize(message),
        _return
    );

    *err = dcapi_response_to_esp_err(res);

    discord_message_t* sent_message = NULL;

    if(_return && *err == ESP_OK) {
        sent_message = discord_model_message_deserialize(res->data, res->data_len);
    }
    
    dcapi_response_free(res);

    return sent_message;
}

discord_message_t* discord_message_send_(discord_client_handle_t client, discord_message_t* message, esp_err_t* err) {
    esp_err_t _err;
    discord_message_t* sent_message = _discord_message_send_(client, message, &_err, true);
    if(err) *err = _err;

    return sent_message;
}

esp_err_t discord_message_send(discord_client_handle_t client, discord_message_t* message) {
    esp_err_t err;
    _discord_message_send_(client, message, &err, false);

    return err;
}