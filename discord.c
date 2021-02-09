#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "discord.h"
#include "esp_websocket_client.h"

static const char* TAG = "discord";

static const discord_bot_config_t* _config;

esp_err_t discord_init(const discord_bot_config_t* config) {
    _config = config;
    return ESP_OK;
}

static void websocket_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data)
{
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
            }
            break;
        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGW(TAG, "WEBSOCKET_EVENT_ERROR");
            break;
    }
}

esp_err_t discord_login() {
    esp_websocket_client_config_t ws_cfg = {
        .uri = "wss://gateway.discord.gg/?v=8&encoding=json"
    };

    esp_websocket_client_handle_t client = esp_websocket_client_init(&ws_cfg);
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void*) client);
    esp_websocket_client_start(client);

    return ESP_OK;
}