#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "cJSON.h"
#include "esp_timer.h"
#include "esp_websocket_client.h"
#include "discord_gateway.h"

static const char* TAG = "discord_gateway";

typedef enum {
    DISCORD_GATEWAY_STATE_ERROR = -1,
    DISCORD_GATEWAY_STATE_UNKNOWN = 0,
    DISCORD_GATEWAY_STATE_INIT,
    DISCORD_GATEWAY_STATE_READY
} discord_gateway_state_t;

struct discord_gateway {
    discord_gateway_state_t state;
    discord_bot_config_t bot;
    esp_websocket_client_handle_t ws;
    esp_timer_handle_t heartbeat_timer;
    discord_gateway_session_t* session;
};

static void heartbeat_timer_callback(void* arg);
static esp_err_t heartbeat_init(discord_gateway_handle_t gateway);
static esp_err_t heartbeat_start(discord_gateway_handle_t gateway, int interval);
static esp_err_t heartbeat_stop(discord_gateway_handle_t gateway);

static esp_err_t gw_push(discord_gateway_handle_t gateway, const char* payload) {
    ESP_LOGI(TAG, "Sending=%s", payload);
    esp_websocket_client_send_text(gateway->ws, payload, strlen(payload), portMAX_DELAY);
    return ESP_OK;
}

static esp_err_t identify(discord_gateway_handle_t gateway) {
    cJSON* props = cJSON_CreateObject();
    cJSON_AddStringToObject(props, "$os", "freertos");
    cJSON_AddStringToObject(props, "$browser", "esp-idf");
    cJSON_AddStringToObject(props, "$device", "esp32");

    cJSON* data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "token", gateway->bot.token);
    cJSON_AddNumberToObject(data, "intents", gateway->bot.intents);
    cJSON_AddItemToObject(data, "properties", props);
    
    cJSON* payload = cJSON_CreateObject();
    cJSON_AddNumberToObject(payload, "op", 2);
    cJSON_AddItemToObject(payload, "d", data);

    char* payload_raw = cJSON_PrintUnformatted(payload);

    cJSON_Delete(payload);

    gw_push(gateway, payload_raw);
    free(payload_raw);

    return ESP_OK;
}

static esp_err_t process_event(discord_gateway_handle_t gateway, const cJSON* payload) {
    cJSON* t = cJSON_GetObjectItem(payload, "t");
    
    if(cJSON_IsNull(t)) {
        ESP_LOGW(TAG, "Missing event name");
        return ESP_FAIL;
    }

    char* event_name = t->valuestring;

    if(strcmp("READY", event_name) == 0) {
        ESP_LOGI(TAG, "Identified.");

        if(gateway->session != NULL) {
            discord_model_gateway_session_free(gateway->session);
        }

        gateway->session = discord_model_gateway_session(cJSON_GetObjectItem(payload, "d"));
        gateway->state = DISCORD_GATEWAY_STATE_READY;
        
        ESP_LOGW(TAG, "Welcome %s#%s [%s], (session: %s)", 
            gateway->session->user->username,
            gateway->session->user->discriminator,
            gateway->session->user->id,
            gateway->session->session_id
        );
    } else if(strcmp("MESSAGE_CREATE", event_name) == 0) {
        ESP_LOGI(TAG, "Received discord message");
    } else {
        ESP_LOGW(TAG, "Unprocessed event \"%s\"", event_name);
    }

    return ESP_OK;
}

static esp_err_t parse_payload(discord_gateway_handle_t gateway, esp_websocket_event_data_t* data) {
    cJSON* payload = cJSON_Parse(data->data_ptr);
    int op = cJSON_GetObjectItem(payload, "op")->valueint;
    ESP_LOGW(TAG, "OP: %d", op);

    if(op == 0) { // event
        process_event(gateway, payload);
    } else if(op == 10) { // heartbeat and identify
        cJSON* d = cJSON_GetObjectItem(payload, "d");
        int heartbeat_interval = cJSON_GetObjectItem(d, "heartbeat_interval")->valueint;

        heartbeat_start(gateway, heartbeat_interval);

        cJSON_Delete(payload);
        payload = NULL;

        identify(gateway);
    } else if(op == 11) {// heartbeat ack
        // TODO:
        // After heartbeat is sent, server need to response with OP 11 (heartbeat ACK)
        // If a client does not receive a heartbeat ack between its attempts at sending heartbeats, 
        // it should immediately terminate the connection with a non-1000 close code,
        // reconnect, and attempt to resume.
    } else {
        ESP_LOGW(TAG, "Unknown OP code %d", op);
    }

    if(payload != NULL) {
        cJSON_Delete(payload);
    }

    return ESP_OK;
}

static void websocket_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
    discord_gateway_handle_t gateway = (discord_gateway_handle_t) handler_args;
    esp_websocket_event_data_t* data = (esp_websocket_event_data_t*) event_data;

    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGW(TAG, "WEBSOCKET_EVENT_CONNECTED");
            gateway->state = DISCORD_GATEWAY_STATE_INIT;
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
            gateway->state = DISCORD_GATEWAY_STATE_UNKNOWN;
            break;
        case WEBSOCKET_EVENT_DATA:
            if(data->op_code == 1) {
                ESP_LOGI(TAG, "Total payload length=%d, data_len=%d, current payload offset=%d", data->payload_len, data->data_len, data->payload_offset);
                ESP_LOGI(TAG, "Received=%.*s", data->data_len, (char*) data->data_ptr);

                if(data->payload_offset > 0 || data->payload_len > 1024) {
                    ESP_LOGE(TAG, "Payload too big. Buffering not implemented yet. Parsing skipped.");
                    break;
                }

                parse_payload(gateway, data);
            }
            break;
        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGW(TAG, "WEBSOCKET_EVENT_ERROR");
            gateway->state = DISCORD_GATEWAY_STATE_ERROR;
            break;
        case WEBSOCKET_EVENT_CLOSED:
            ESP_LOGW(TAG, "WEBSOCKET_EVENT_CLOSED");
            gateway->state = DISCORD_GATEWAY_STATE_UNKNOWN;
            break;
        default:
            ESP_LOGE(TAG, "WEBSOCKET_EVENT_UNKNOWN %d", event_id);
            break;
    }
}

discord_gateway_handle_t discord_gw_init(const discord_gateway_config_t* config) {
    discord_gateway_handle_t gateway = calloc(1, sizeof(struct discord_gateway));

    gateway->bot = config->bot;

    return gateway;
}

esp_err_t discord_gw_open(discord_gateway_handle_t gateway) {
    if(gateway == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if(gateway->state >= DISCORD_GATEWAY_STATE_INIT) {
        ESP_LOGE(TAG, "The gateway has been already open");
        return ESP_FAIL;
    }

    esp_websocket_client_config_t ws_cfg = {
        .uri = "wss://gateway.discord.gg/?v=8&encoding=json"
    };

    gateway->ws = esp_websocket_client_init(&ws_cfg);

    ESP_ERROR_CHECK(heartbeat_init(gateway));
    ESP_ERROR_CHECK(esp_websocket_register_events(gateway->ws, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void*) gateway));
    ESP_ERROR_CHECK(esp_websocket_client_start(gateway->ws));

    return ESP_OK;
}

esp_err_t discord_gw_destroy(discord_gateway_handle_t gateway) {
    heartbeat_stop(gateway);
    esp_timer_delete(gateway->heartbeat_timer);
    gateway->heartbeat_timer = NULL;

    esp_websocket_client_destroy(gateway->ws);
    gateway->ws = NULL;

    discord_model_gateway_session_free(gateway->session);
    gateway->session = NULL;

    free(gateway);

    return ESP_OK;
}

// Heartbeat

static void heartbeat_timer_callback(void* arg) {
    ESP_LOGI(TAG, "Heartbeating...");

    discord_gateway_handle_t gateway = (discord_gateway_handle_t) arg;
    gw_push(gateway, "{ \"op\": 1, \"d\": null }");
}

static esp_err_t heartbeat_init(discord_gateway_handle_t gateway) {
    ESP_LOGI(TAG, "Heartbeat init");

    const esp_timer_create_args_t timer_args = {
        .callback = &heartbeat_timer_callback,
        .arg = (void*) gateway
    };

    return esp_timer_create(&timer_args, &(gateway->heartbeat_timer));
}

static esp_err_t heartbeat_start(discord_gateway_handle_t gateway, int interval) {
    ESP_LOGI(TAG, "Heartbeat start");
    return esp_timer_start_periodic(gateway->heartbeat_timer, interval * 1000);
}

static esp_err_t heartbeat_stop(discord_gateway_handle_t gateway) {
    ESP_LOGI(TAG, "Heartbeat stop");
    return esp_timer_stop(gateway->heartbeat_timer);
}