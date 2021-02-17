#include "discord/private/_discord.h"
#include "discord/private/_api.h"
#include "esp_http_client.h"

DISCORD_LOG_DEFINE_BASE();

bool dcapi_response_is_success(discord_api_response_t* res) {
    if(res == NULL)
        return false;
    
    return res->code >= 200 && res->code <= 299;
}

void dcapi_response_free(discord_api_response_t* res) {
    if(res == NULL)
        return;
    
    free(res->data);
    free(res);
}

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

static discord_api_response_t* dcapi_request_fail(discord_client_handle_t client) {
    DISCORD_LOG_FOO();

    if(! DISCORD_API_KEEPALIVE) {
        esp_http_client_close(client->http);
    }

    return NULL;
}

static discord_api_response_t* dcapi_request(discord_client_handle_t client, esp_http_client_method_t method, const char* uri, const char* data) {
    DISCORD_LOG_FOO();

    if(dcapi_init_lazy(client) != ESP_OK) { // will just return ESP_OK if already inited
        DISCORD_LOGW("Cannot initialize API");
        return NULL;
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

    if(esp_http_client_open(http, len) != ESP_OK) {
        DISCORD_LOGW("Failed to open connection with API");
        return NULL;
    }

    DISCORD_LOGD("Sending request to API:\n%.*s", len, data);

    if(esp_http_client_write(http, data, len) == ESP_FAIL) {
        DISCORD_LOGW("Failed to send data to API");
        return dcapi_request_fail(client);
    }

    DISCORD_LOGD("Fetching API response headers...");

    if(esp_http_client_fetch_headers(http) == ESP_FAIL) {
        DISCORD_LOGW("Fail to fetch API response");
        return dcapi_request_fail(client);
    }

    esp_http_client_flush_response(http, NULL);

    discord_api_response_t* res = calloc(1, sizeof(discord_api_response_t));

    res->code = esp_http_client_get_status_code(http);

    DISCORD_LOGD("Received response from API with res_code %d", res->code);

    if(! DISCORD_API_KEEPALIVE) {
        esp_http_client_close(http);
    }

    return res;
}

discord_api_response_t* dcapi_post(discord_client_handle_t client, const char* uri, const char* data) {
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