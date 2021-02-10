#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "cJSON.h"
#include "esp_timer.h"
#include "esp_websocket_client.h"
#include "discord_gateway.h"
#include "discord_gateway_heartbeat.h"

static const char* TAG = "discord_gateway";

static discord_bot_config_t _config;
static esp_websocket_client_handle_t ws_client;

static esp_err_t discord_gw_identify() {
    cJSON* props = cJSON_CreateObject();
    cJSON_AddStringToObject(props, "$os", "freertos");
    cJSON_AddStringToObject(props, "$browser", "esp-idf");
    cJSON_AddStringToObject(props, "$device", "esp32");

    cJSON* data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "token", _config.token);
    cJSON_AddNumberToObject(data, "intents", 512);
    cJSON_AddItemToObject(data, "properties", props);
    
    cJSON* payload = cJSON_CreateObject();
    cJSON_AddNumberToObject(payload, "op", 2);
    cJSON_AddItemToObject(payload, "d", data);

    char* payload_raw = cJSON_PrintUnformatted(payload);

    cJSON_Delete(payload);

    ESP_LOGI(TAG, "Sending=%s", payload_raw);

    esp_websocket_client_send_text(ws_client, payload_raw, strlen(payload_raw), portMAX_DELAY);

    free(payload_raw);

    return ESP_OK;
}

static esp_err_t discord_gw_process_event(const cJSON* payload) {
    cJSON* t = cJSON_GetObjectItem(payload, "t");
    
    if(cJSON_IsNull(t)) {
        ESP_LOGW(TAG, "Missing event name");
        return ESP_FAIL;
    }

    char* event_name = t->valuestring;

    if(strcmp("READY", event_name) == 0) {
        ESP_LOGI(TAG, "Successfully identified");
    } else if(strcmp("MESSAGE_CREATE", event_name) == 0) {
        ESP_LOGI(TAG, "Received discord message");
    }

    return ESP_OK;
}

static esp_err_t discord_gw_parse_payload(esp_websocket_event_data_t* data) {
    cJSON* payload = cJSON_Parse(data->data_ptr);
    int op = cJSON_GetObjectItem(payload, "op")->valueint;
    ESP_LOGW(TAG, "OP: %d", op);
    cJSON* d;
    int heartbeat_interval;

    switch(op) {
        case 0: // event
            discord_gw_process_event(payload);
            break;
        case 10: // heartbeat and identify
            d = cJSON_GetObjectItem(payload, "d");
            heartbeat_interval = cJSON_GetObjectItem(d, "heartbeat_interval")->valueint;
            discord_gw_heartbeat_start(heartbeat_interval);
            discord_gw_identify();
            break;
        case 11: // heartbeat ack
            // TODO:
            // After heartbeat is sent, server need to response with OP 11 (heartbeat ACK)
            // If a client does not receive a heartbeat ack between its attempts at sending heartbeats, 
            // it should immediately terminate the connection with a non-1000 close code,
            // reconnect, and attempt to resume.
            break;
        default:
            ESP_LOGW(TAG, "Unknown OP code %d", op);
            break;
    }

    cJSON_Delete(payload);
    return ESP_OK;
}

static void websocket_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
    esp_websocket_event_data_t* data = (esp_websocket_event_data_t*) event_data;

    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGW(TAG, "WEBSOCKET_EVENT_CONNECTED");
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
            break;
        case WEBSOCKET_EVENT_DATA:
            if(data->op_code == 1) {
                ESP_LOGI(TAG, "Total payload length=%d, data_len=%d, current payload offset=%d", data->payload_len, data->data_len, data->payload_offset);
                ESP_LOGI(TAG, "Received=%.*s", data->data_len, (char*) data->data_ptr);

                discord_gw_parse_payload(data);
            }
            break;
        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGW(TAG, "WEBSOCKET_EVENT_ERROR");
            break;
        case WEBSOCKET_EVENT_CLOSED:
            ESP_LOGW(TAG, "WEBSOCKET_EVENT_CLOSED");
            break;
        default:
            ESP_LOGW(TAG, "WEBSOCKET_EVENT_UNKNOWN %d", event_id);
            break;
    }
}

esp_err_t discord_gw_init(const discord_bot_config_t config) {
    _config = config;
    return ESP_OK;
}

esp_err_t discord_gw_open() {
    esp_websocket_client_config_t ws_cfg = {
        .uri = "wss://gateway.discord.gg/?v=8&encoding=json"
    };

    ws_client = esp_websocket_client_init(&ws_cfg);
    
    ESP_ERROR_CHECK(discord_gw_heartbeat_init(ws_client));

    esp_websocket_register_events(ws_client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void*) ws_client);
    esp_websocket_client_start(ws_client);

    return ESP_OK;
}