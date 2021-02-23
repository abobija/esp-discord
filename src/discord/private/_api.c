#include "discord/utils.h"
#include "discord/private/_discord.h"
#include "discord/private/_api.h"
#include "esp_http_client.h"

DISCORD_LOG_DEFINE_BASE();

bool dcapi_response_is_success(discord_api_response_t* res) {
    return res && res->code >= 200 && res->code <= 299;
}

bool dcapi_response_is_client_error(discord_api_response_t* res) {
    return res && res->code >= 400 && res->code <= 499;
}

esp_err_t dcapi_response_to_esp_err(discord_api_response_t* res) {
    return res && dcapi_response_is_success(res) ? ESP_OK : ESP_FAIL;
}

void dcapi_response_free(discord_handle_t client, discord_api_response_t* res) {
    if(res == NULL)
        return;

    if(res->data || res->data_len > 0) { // if buffer was attached to result
        client->api_buffer_size = 0;
    }

    res->data = NULL; // do not free() res->data because it holds addr of internal api buffer
    res->data_len = 0;
    free(res);
}

static esp_err_t dcapi_flush_http(discord_handle_t client, bool record) {
    DISCORD_LOG_FOO();

    client->api_buffer_record = record;
    esp_err_t err = esp_http_client_flush_response(client->http, NULL);
    client->api_buffer_record = false;

    if(! record) {
        client->api_buffer_size = 0;
    }

    return err;
}

static esp_err_t dcapi_on_http_event(esp_http_client_event_t* evt) {
    discord_handle_t client = (discord_handle_t) evt->user_data;

    if(evt->event_id == HTTP_EVENT_ON_DATA && evt->data_len > 0 && client->api_buffer_record && client->api_buffer) {
        DISCORD_LOGD("Buffering received chunk data_len=%d:\n%.*s", evt->data_len, evt->data_len, (char*) evt->data);

        if(client->api_buffer_size + evt->data_len > DISCORD_API_BUFFER_SIZE) { // prevent buffer overflow
            DISCORD_LOGW("Response chunk cannot fit into api buffer");
            client->api_buffer_record_status = ESP_FAIL;
            client->api_buffer_size = 0;
            return ESP_FAIL;
        }

        memcpy(client->api_buffer + client->api_buffer_size, evt->data, evt->data_len);

        client->api_buffer_size += evt->data_len;
    }

    return ESP_OK;
}

static esp_err_t dcapi_init_lazy(discord_handle_t client) {
    if(client->http != NULL)
        return ESP_OK;

    DISCORD_LOG_FOO();

    if(client->state < DISCORD_STATE_CONNECTED) {
        DISCORD_LOGW("API can be initialized only if client is in CONNECTED state");
        return ESP_FAIL;
    }

    esp_http_client_config_t config = {
        .url = DISCORD_API_URL,
        .is_async = false,
        .keep_alive_enable = true,
        .event_handler = dcapi_on_http_event,
        .user_data = client,
        .timeout_ms = DISCORD_API_TIMEOUT_MS
    };

    client->api_buffer_record_status = ESP_OK;

    if(!(client->api_lock = xSemaphoreCreateMutex()) ||
       !(client->api_buffer = malloc(DISCORD_API_BUFFER_SIZE)) ||
       !(client->http = esp_http_client_init(&config))) {
        DISCORD_LOGW("Cannot allocate api. No memory.");
        dcapi_destroy(client);
        return ESP_FAIL;
    }

    char* auth = discord_strcat("Bot ", client->config->token);
    esp_http_client_set_header(client->http, "Authorization", auth);
    free(auth);

    esp_http_client_set_header(client->http, "User-Agent", "DiscordBot (ESP-IDF, 1.0.0.0)");
    esp_http_client_set_header(client->http, "Content-Type", "application/json");

    return client->http ? ESP_OK : ESP_FAIL;
}

static discord_api_response_t* dcapi_request(discord_handle_t client, esp_http_client_method_t method, char* uri, char* data, bool stream_response, bool free_uri_and_data) {
    DISCORD_LOG_FOO();

    if(dcapi_init_lazy(client) != ESP_OK) { // will just return ESP_OK if already initialized
        DISCORD_LOGW("Cannot initialize API");
        return NULL;
    }

    if(xSemaphoreTake(client->api_lock, DISCORD_API_TIMEOUT_MS / portTICK_PERIOD_MS) != pdTRUE) {
        DISCORD_LOGW("Api is locked");
        return NULL;
    }

    esp_http_client_handle_t http = client->http;

    client->api_buffer_record = true; // always record first chunk which comes with headers because maybe will need to record error
    client->api_buffer_record_status = ESP_OK;

    char* url = discord_strcat(DISCORD_API_URL, uri);
    if(free_uri_and_data) { free(uri); }
    esp_http_client_set_url(http, url);
    free(url);

    esp_http_client_set_method(http, method);

    int len = data ? strlen(data) : 0;

    if(esp_http_client_open(http, len) != ESP_OK) {
        DISCORD_LOGW("Failed to open connection");
        xSemaphoreGive(client->api_lock);
        return NULL;
    }

    if(data) {
        DISCORD_LOGD("Writing data to request:\n%.*s", len, data);

        if(esp_http_client_write(http, data, len) == ESP_FAIL) {
            DISCORD_LOGW("Fail to write data to request");
            xSemaphoreGive(client->api_lock);
            return NULL;
        }

        if(free_uri_and_data) { free(data); }
    }

    DISCORD_LOGD("Sending request and fetching response...");

    if(esp_http_client_fetch_headers(http) == ESP_FAIL) {
        DISCORD_LOGW("Fail to fetch headers");
        dcapi_flush_http(client, false);
        xSemaphoreGive(client->api_lock);
        return NULL;
    }

    discord_api_response_t* res = calloc(1, sizeof(discord_api_response_t));

    res->code = esp_http_client_get_status_code(http);
    res->data_len = 0;

    bool is_error = ! dcapi_response_is_success(res);

    dcapi_flush_http(client, stream_response || is_error);  // record if stream_response is true or there is errors

    if(stream_response || is_error) {
        if(client->api_buffer_record_status != ESP_OK) {
            DISCORD_LOGW("Fail to record response chunks");
            client->api_buffer_size = 0;
        } else if(! is_error) { // point response to buffer if there is no errors
            res->data = client->api_buffer;
            res->data_len = client->api_buffer_size;
        }
    }

    DISCORD_LOGD("Received api response (res_code=%d, data_len=%d)", res->code, res->data_len);

    if(res->data_len > 0) {
        DISCORD_LOGD("%.*s", res->data_len, res->data);
    }

    if(is_error && client->api_buffer_size > 0) {
        DISCORD_LOGW("Error: %.*s", client->api_buffer_size, client->api_buffer); // just print raw error for now
        client->api_buffer_size = 0;
    }

    xSemaphoreGive(client->api_lock);

    return res;
}

discord_api_response_t* dcapi_post(discord_handle_t client, char* uri, char* data, bool stream) {
    return dcapi_request(client, HTTP_METHOD_POST, uri, data, stream, true);
}

esp_err_t dcapi_post_(discord_handle_t client, char* uri, char* data) {
    discord_api_response_t* res = dcapi_post(client, uri, data, false);
    esp_err_t err = dcapi_response_to_esp_err(res);
    dcapi_response_free(client, res);

    return err;
}

discord_api_response_t* dcapi_put(discord_handle_t client, char* uri, char* data, bool stream) {
    return dcapi_request(client, HTTP_METHOD_PUT, uri, data, stream, true);
}

esp_err_t dcapi_put_(discord_handle_t client, char* uri, char* data) {
    discord_api_response_t* res = dcapi_put(client, uri, data, false);
    esp_err_t err = dcapi_response_to_esp_err(res);
    dcapi_response_free(client, res);

    return err;
}

esp_err_t dcapi_destroy(discord_handle_t client) {
    DISCORD_LOG_FOO();

    if(client->api_lock) {
        xSemaphoreTake(client->api_lock, portMAX_DELAY); // wait for unlock
        vSemaphoreDelete(client->api_lock);
        client->api_lock = NULL;
    }

    if(client->api_buffer != NULL) {
        free(client->api_buffer);
        client->api_buffer = NULL;
    }

    client->api_buffer_size = 0;
    client->api_buffer_record = false;

    if(client->http) {
        esp_http_client_cleanup(client->http);
        client->http = NULL;
    }

    return ESP_OK;
}

char* dcapi_urlencode(const char* str) {
    if(!str) { return NULL; }
    static char hex[] = "0123456789abcdef";
    size_t _len = strlen(str);
    char* buf = malloc(_len * 3 + 1);
    if(!buf) { return NULL; }
    char* pbuf = buf;

    for(size_t i = 0; i < _len; i++) {
        if (isalnum(str[i]) || strchr(".-_~", str[i])) {
            *pbuf++ = str[i];
        } else if (str[i] == ' ') {
            *pbuf++ = '+';
        } else {
            *pbuf++ = '%';
            *pbuf++ = hex[(str[i] >> 4) & 15];
            *pbuf++ = hex[str[i] & 15];
        }
    }

    *pbuf = '\0';

    return buf;
}