#include "discord/private/_discord.h"
#include "discord/private/_api.h"
#include "esp_http_client.h"

DISCORD_LOG_DEFINE_BASE();

static esp_err_t dcapi_init_lazy(discord_client_handle_t client) {
    if(client->http != NULL)
        return ESP_OK;

    DISCORD_LOG_FOO();

    if(client->state < DISCORD_CLIENT_STATE_CONNECTED) {
        DISCORD_LOGW("API can be initialized only if client is in CONNECTED state");
        return ESP_FAIL;
    }

    esp_http_client_config_t config = {
        .url = DISCORD_API_URL,
        .is_async = false,
        .keep_alive_enable = DISCORD_API_KEEPALIVE
    };

    client->http = esp_http_client_init(&config);

    return client->http ? ESP_OK : ESP_FAIL;
}

static esp_err_t dcapi_request(discord_client_handle_t client, esp_http_client_method_t method, const char* uri, const char* data) {
    DISCORD_LOG_FOO();

    if(dcapi_init_lazy(client) != ESP_OK) { // will just return ESP_OK if already inited
        DISCORD_LOGW("Cannot initialize API");
        return ESP_FAIL;
    }

    esp_http_client_handle_t http = client->http;

    char* url = STRCAT(DISCORD_API_URL, uri);
    esp_http_client_set_url(http, url);
    free(url);

    esp_http_client_set_method(http, HTTP_METHOD_POST);

    char* auth = STRCAT("Bot ", client->config->token);
    esp_http_client_set_header(http, "Authorization", auth);
    free(auth);

    esp_http_client_set_header(http, "User-Agent", "DiscordBot (ESP-IDF, 1.0.0.0)");
    esp_http_client_set_header(http, "Content-Type", "application/json");

    int len = strlen(data);

    if(esp_http_client_open(http, len) == ESP_OK) {
        esp_http_client_write(http, data, len);

        if(! DISCORD_API_KEEPALIVE) {
            esp_http_client_close(http);
        }
    } else {
        DISCORD_LOGW("Failed to open connection with API");
    }

    return ESP_OK;
}

esp_err_t dcapi_post(discord_client_handle_t client, const char* uri, const char* data) {
    return dcapi_request(client, HTTP_METHOD_POST, uri, data);
}

esp_err_t dcapi_close(discord_client_handle_t client) {
    DISCORD_LOG_FOO();

    if(client->http) {
        esp_http_client_cleanup(client->http);
        client->http = NULL;
    }

    return ESP_OK;
}