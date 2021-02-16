#include "discord/private/_discord.h"
#include "discord/private/_api.h"
#include "discord/models.h"
#include "esp_http_client.h"

DISCORD_LOG_DEFINE_BASE();

esp_err_t dcapi_init(discord_client_handle_t client) {
    DISCORD_LOG_FOO();
    return ESP_OK;
}

esp_err_t dcapi_destroy(discord_client_handle_t client) {
    DISCORD_LOG_FOO();
    return ESP_OK;
}

esp_err_t dcapi_send_message(discord_client_handle_t client, discord_message_t* msg) {
    DISCORD_LOG_FOO();

    char* url_tmp = dc_strcat("https://discord.com/api/v8/channels/", msg->channel_id);
    char* url = dc_strcat(url_tmp, "/messages");
    free(url_tmp);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .is_async = false
    };

    esp_http_client_handle_t http = esp_http_client_init(&config);
    free(url);

    char* auth = dc_strcat("Bot ", client->config->token);
    esp_http_client_set_header(http, "Authorization", auth);
    free(auth);

    esp_http_client_set_header(http, "User-Agent", "DiscordBot (ESP-IDF, 1.0.0.0)");
    esp_http_client_set_header(http, "Content-Type", "application/json");

    char* buff = "{ \"content\": \"hi\" }";
    int len = strlen(buff);

    esp_http_client_open(http, len);
    esp_http_client_write(http, buff, len);
    esp_http_client_close(http);

    esp_http_client_cleanup(http);

    return ESP_OK;
}