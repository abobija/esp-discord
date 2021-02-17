#include "discord/messages.h"
#include "discord/private/_discord.h"
#include "discord/private/_api.h"

DISCORD_LOG_DEFINE_BASE();

esp_err_t discord_message_send(discord_client_handle_t client, discord_message_t* message) {
    if(message == NULL)
        return ESP_ERR_INVALID_ARG;

    DISCORD_LOG_FOO();

    if(message->content == NULL) {
        DISCORD_LOGE("Missing content");
        return ESP_ERR_INVALID_ARG;
    }

    if(message->channel_id == NULL) {
        DISCORD_LOGE("Missing channel_id");
        return ESP_ERR_INVALID_ARG;
    }

    discord_api_response_t* res = DCAPI_POST(
        STRCAT("/channels/", message->channel_id, "/messages"),
        discord_model_message_serialize(message)
    );

    esp_err_t err = res && dcapi_response_is_success(res) ? ESP_OK : ESP_FAIL;

    dcapi_response_free(res);

    return err;
}