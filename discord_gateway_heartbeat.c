#include "esp_system.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "discord_gateway_heartbeat.h"
#include "esp_websocket_client.h"

static const char* TAG = "discord_gateway_heartbeat";

static esp_websocket_client_handle_t _ws_client;
static esp_timer_handle_t timer;

static void timer_callback(void* arg) {
    ESP_LOGI(TAG, "Heartbeating...");

    const char* payload_raw = "{ \"op\": 1, \"d\": null }";
    ESP_LOGI(TAG, "Sending=%s", payload_raw);
    esp_websocket_client_send_text(_ws_client, payload_raw, strlen(payload_raw), portMAX_DELAY);
}

esp_err_t discord_gw_heartbeat_init(esp_websocket_client_handle_t ws_client) {
    _ws_client = ws_client;

    const esp_timer_create_args_t timer_args = {
        .callback = &timer_callback
    };

    return esp_timer_create(&timer_args, &timer);
}

esp_err_t discord_gw_heartbeat_start(int interval) {
    ESP_LOGI(TAG, "Heartbeat starting");
    return esp_timer_start_periodic(timer, interval * 1000);
}