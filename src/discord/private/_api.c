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
    res->data_len = 0;
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

static char* dcapi_empty_http_data(char* buffer, int** len, int len_val) {
    if(buffer) {
        free(buffer);
    }

    if(len) {
        **len = len_val;
    }

    return NULL;
}

/**
 * @param len On success it will be >= 0, otherwise it will become -1 if there is no Content-Length header in response or -2 on memory or http_read error
 * @return Pointer to data buffer if Content-Lenght header is > 0, otherwise NULL
 */
static char* dcapi_read_http_response_using_content_length_header(discord_client_handle_t client, int* len) {
    DISCORD_LOG_FOO();

    int content_length = esp_http_client_get_content_length(client->http);

    if(content_length < 0) {
        return dcapi_empty_http_data(NULL, &len, -1);
    }

    if(content_length == 0) {
        return dcapi_empty_http_data(NULL, &len, 0);
    }

    if(content_length > DISCORD_API_BUFFER_MAX_SIZE) {
        DISCORD_LOGW("Api response body is bigger than the DISCORD_API_BUFFER_MAX_SIZE");
        return dcapi_empty_http_data(NULL, &len, -2);
    }

    char* buffer = malloc(*len = content_length + 1); // (+1 for null terminator)

    if(buffer == NULL) { // malloc error
        return dcapi_empty_http_data(buffer, &len, -2);
    }

    if(esp_http_client_read(client->http, buffer, content_length) < 0) {
        return dcapi_empty_http_data(buffer, &len, -2);
    }

    buffer[content_length] = '\0';

    return buffer;
}

/**
 * @param len On success it will be >= 0, otherwise ESP_FAIL (-1)
 * @return Pointer to data buffer if there is data in http response, otherwise NULL
 */
static char* dcapi_read_http_response_stream(discord_client_handle_t client, int* len) {
    DISCORD_LOG_FOO();

    int _len;
    char* buffer = dcapi_read_http_response_using_content_length_header(client, &_len);

    if(_len >= 0) {
        *len = _len;
        return buffer;
    }

    if(_len == -2) { // memory or http error
        return dcapi_empty_http_data(buffer, &len, ESP_FAIL);
    }

    // content-length is < 0
    // need to dinamically read http stream
    
    DISCORD_LOGD("Dinamically read http stream...");

    const int step_expand = 256;
    int step = 0, read_len = 0, data_read = 0;
    buffer = calloc(0, sizeof(char));
    
    do {
        if(read_len + step_expand > DISCORD_API_BUFFER_MAX_SIZE) { // response is bigger that the max buffer size
            DISCORD_LOGW("Api response body is bigger than the DISCORD_API_BUFFER_MAX_SIZE");
            return dcapi_empty_http_data(buffer, &len, ESP_FAIL);
        }

        char* _buffer = realloc(buffer, ++step * step_expand); // expand

        if(_buffer == NULL) { // expand error
            return dcapi_empty_http_data(buffer, &len, ESP_FAIL);
        }

        buffer = _buffer;

        data_read = esp_http_client_read(client->http, buffer + read_len, step_expand);
        read_len += data_read;

        if(data_read < 0) { // error http read
            return dcapi_empty_http_data(buffer, &len, ESP_FAIL);
        }
        
        if(data_read < step_expand) { // last chunk
            if(read_len == 0) { // no data in stream
                return dcapi_empty_http_data(buffer, &len, 0);
            }

            _buffer = realloc(buffer, ++read_len); // shrink (+1 for null terminator)

            if(_buffer == NULL) { // shrink error
                return dcapi_empty_http_data(buffer, &len, ESP_FAIL);
            }

            buffer = _buffer;
            buffer[read_len - 1] = '\0';
        }
    } while(data_read >= step_expand);

    *len = read_len;

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

    if(esp_http_client_fetch_headers(http) == ESP_FAIL) {
        DISCORD_LOGW("Fail to fetch API response");
        return dcapi_request_fail(client);
    }

    discord_api_response_t* res = calloc(1, sizeof(discord_api_response_t));

    res->code = esp_http_client_get_status_code(http);
    res->data_len = 0;

    if(stream_response) {
        res->data = dcapi_read_http_response_stream(client, &res->data_len);

        if(res->data_len < 0) {
            DISCORD_LOGW("Failed to read http stream");
        }
    }

    esp_http_client_flush_response(http, NULL);

    DISCORD_LOGD("Received response from API (res_code=%d, data_len=%d)", res->code, res->data_len);

    if(stream_response && res->data_len > 0) {
        DISCORD_LOGD("%.*s", res->data_len, res->data);
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

    if(client->http) {
        esp_http_client_cleanup(client->http);
        client->http = NULL;
    }

    return ESP_OK;
}