#include "discord/private/_discord.h"
#include "discord/private/_api.h"
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

esp_err_t dcapi_post(discord_client_handle_t client, const char* uri, const char* data) {
    DISCORD_LOG_FOO();

    char* url = STRCAT("https://discord.com/api/v8", uri);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .is_async = false
    };

    esp_http_client_handle_t http = esp_http_client_init(&config);
    free(url);

    char* auth = STRCAT("Bot ", client->config->token);
    esp_http_client_set_header(http, "Authorization", auth);
    free(auth);

    esp_http_client_set_header(http, "User-Agent", "DiscordBot (ESP-IDF, 1.0.0.0)");
    esp_http_client_set_header(http, "Content-Type", "application/json");

    int len = strlen(data);

    esp_http_client_open(http, len);
    esp_http_client_write(http, data, len);
    esp_http_client_close(http);
    esp_http_client_cleanup(http);

    return ESP_OK;
}