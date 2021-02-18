#include "discord/private/_discord.h"
#include "discord/private/_api.h"
#include "esp_http_client.h"

DISCORD_LOG_DEFINE_BASE();

bool dcapi_response_is_success(discord_api_response_t* res) {
    if(res == NULL)
        return false;
    
    return res->code >= 200 && res->code <= 299;
}

esp_err_t dcapi_response_to_esp_err(discord_api_response_t* res) {
    return res && dcapi_response_is_success(res) ? ESP_OK : ESP_FAIL;
}

void dcapi_response_free(discord_api_response_t* res) {
    if(res == NULL)
        return;
    
    free(res->data);
    res->data = NULL;
    res->data_len = 0;
    free(res);
}

static void dcapi_clear_buffer(discord_client_handle_t client) {
    DISCORD_LOG_FOO();

    if(client->http_buffer != NULL) {
        free(client->http_buffer);
        client->http_buffer = NULL;
    }
    
    client->http_buffer_size = 0;
    client->http_buffer_record = false;
}

static esp_err_t dcapi_flush_http(discord_client_handle_t client, bool record) {
    DISCORD_LOG_FOO();

    client->http_buffer_record = record;
    esp_err_t err = esp_http_client_flush_response(client->http, NULL);
    client->http_buffer_record = false;

    if(! record) {
        dcapi_clear_buffer(client);
    }

    return err;
}

static esp_err_t dcapi_on_http_event(esp_http_client_event_t* evt) {
    discord_client_handle_t client = (discord_client_handle_t) evt->user_data;

    if(evt->event_id == HTTP_EVENT_ON_DATA && client->http_buffer_record && evt->data_len > 0) {
        DISCORD_LOGD("http chunk received data_len=%d:\n%.*s", evt->data_len, evt->data_len, (char*) evt->data);

        int old_size = client->http_buffer_size;
        int new_size = old_size + evt->data_len;

        if(new_size > DISCORD_API_BUFFER_MAX_SIZE) { // buffer overflow can occured
            DISCORD_LOGW("Api response chunk cannot fit into buffer");
            return ESP_OK;
        }

        char* _buffer = realloc(client->http_buffer, new_size);

        if(_buffer == NULL) {
            DISCORD_LOGW("Cannot expand api buffer");
            return ESP_FAIL;
        }

        memcpy(_buffer + old_size, evt->data, evt->data_len);

        client->http_buffer = _buffer;
        client->http_buffer_size = new_size;
    }

    return ESP_OK;
}

static esp_err_t dcapi_init_lazy(discord_client_handle_t client) {
    if(client->http != NULL)
        return ESP_OK;

    DISCORD_LOG_FOO();

    if(client->state < DISCORD_CLIENT_STATE_CONNECTED) {
        DISCORD_LOGW("API can be initialized only if client is in CONNECTED state");
        return ESP_FAIL;
    }

    dcapi_clear_buffer(client);

    esp_http_client_config_t config = {
        .url = DISCORD_API_URL,
        .is_async = false,
        .keep_alive_enable = DISCORD_API_KEEPALIVE,
        .event_handler = dcapi_on_http_event,
        .user_data = client
    };

    client->http = esp_http_client_init(&config);

    return client->http ? ESP_OK : ESP_FAIL;
}

/**
 * @param len On success it will be >= 0, otherwise ESP_FAIL (-1)
 * @return Pointer to data buffer if there is data in http response, otherwise NULL
 */
static char* dcapi_read_http_response_stream(discord_client_handle_t client, int* len) {
    DISCORD_LOG_FOO();

    dcapi_flush_http(client, true);

    char* buffer = client->http_buffer;
    *len = client->http_buffer_size;

    client->http_buffer = NULL;
    client->http_buffer_size = 0;

    return buffer;
}

static discord_api_response_t* dcapi_request_fail(discord_client_handle_t client) {
    DISCORD_LOG_FOO();

    if(! DISCORD_API_KEEPALIVE) {
        esp_http_client_close(client->http);
    }

    return NULL;
}

static discord_api_response_t* dcapi_request(discord_client_handle_t client, esp_http_client_method_t method, const char* uri, const char* data, bool stream_response) {
    DISCORD_LOG_FOO();

    if(dcapi_init_lazy(client) != ESP_OK) { // will just return ESP_OK if already initialized
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

    DISCORD_LOGD("Fetching API response...");

    client->http_buffer_record = true; // record first chunk which will come with headers

    if(esp_http_client_fetch_headers(http) == ESP_FAIL) {
        DISCORD_LOGW("Fail to fetch API response");
        return dcapi_request_fail(client);
    }

    client->http_buffer_record = false;

    discord_api_response_t* res = calloc(1, sizeof(discord_api_response_t));

    res->code = esp_http_client_get_status_code(http);
    res->data_len = 0;

    if(! stream_response) {
        dcapi_flush_http(client, false);
    } else {
        res->data = dcapi_read_http_response_stream(client, &res->data_len);
        
        if(res->data_len < 0) {
            DISCORD_LOGW("Failed to read http stream");
        }
    }

    DISCORD_LOGD("Received response from API (res_code=%d, data_len=%d)", res->code, res->data_len);

    if(stream_response && res->data_len > 0) {
        for(int i = 0; i < res->data_len; i++) {
            if(res->data[i] >= 32 && res->data[i] <= 126) {
                printf("%c", res->data[i]);
            } else {
                printf("[%02x:%d] ", res->data[i], res->data[i]);
            }
        }
        
        printf("\n");

        //DISCORD_LOGD("%.*s", res->data_len, res->data);
    }

    if(! DISCORD_API_KEEPALIVE) {
        esp_http_client_close(http);
    }

    return res;
}

discord_api_response_t* dcapi_post(discord_client_handle_t client, char* uri, char* data, bool stream) {
    discord_api_response_t* res = dcapi_request(client, HTTP_METHOD_POST, uri, data, stream);

    if(uri) free(uri);
    if(data) free(data);

    return res;
}

esp_err_t dcapi_close(discord_client_handle_t client) {
    DISCORD_LOG_FOO();

    client->http_buffer_record = false;
    dcapi_clear_buffer(client);

    if(client->http) {
        esp_http_client_cleanup(client->http);
        client->http = NULL;
    }

    return ESP_OK;
}