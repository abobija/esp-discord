#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_event_base.h"
#include "esp_websocket_client.h"
#include "_models.h"
#include "discord.h"

#define DISCORD_WS_BUFFER_SIZE (512)
#define DISCORD_MIN_BUFFER_SIZE (1024)
#define DISCORD_TASK_STACK_SIZE (6 * 1024)
#define DISCORD_TASK_PRIORITY (3)

#define DISCORD_LOG_TAG "discord"

#define DISCORD_LOG_DEFINE_BASE() static const char* TAG = DISCORD_LOG_TAG
#define DISCORD_LOG(esp_log_foo, format, ...) esp_log_foo(TAG, "%s: " format, __func__, ##__VA_ARGS__)
#define DISCORD_LOGE(format, ...) DISCORD_LOG(ESP_LOGE, format, ##__VA_ARGS__)
#define DISCORD_LOGW(format, ...) DISCORD_LOG(ESP_LOGW, format, ##__VA_ARGS__)
#define DISCORD_LOGI(format, ...) DISCORD_LOG(ESP_LOGI, format, ##__VA_ARGS__)
#define DISCORD_LOGD(format, ...) DISCORD_LOG(ESP_LOGD, format, ##__VA_ARGS__)
#define DISCORD_LOGV(format, ...) DISCORD_LOG(ESP_LOGV, format, ##__VA_ARGS__)
#define DISCORD_LOG_FOO() DISCORD_LOGD("...")

#define DISCORD_EVENT_EMIT(event, data) client->event_handler(client, event, data)

#define DC_LOCK(code, fail_to_lock_code) {\
    if (xSemaphoreTakeRecursive(client->lock, portMAX_DELAY) != pdPASS) {\
        DISCORD_LOGE("Could not lock");\
        fail_to_lock_code;\
    }\
    code;\
    xSemaphoreGiveRecursive(client->lock);\
}

#define DC_LOCK_NO_ERR(code) DC_LOCK(code, ({;}))
#define DC_LOCK_BREAK(code) DC_LOCK(code, break)
#define DC_LOCK_ESP_ERR(code) DC_LOCK(code, return ESP_FAIL)

#define STREQ(s1, s2) (strcmp(s1, s2) == 0)
#define STRDUP(str) (str ? strdup(str) : NULL)
#define STRCAT(...) _dc_strcat(__VA_ARGS__, NULL)

typedef enum {
    DISCORD_CLOSE_REASON_NOT_REQUESTED,
    DISCORD_CLOSE_REASON_HEARTBEAT_ACK_NOT_RECEIVED,
    DISCORD_CLOSE_REASON_LOGOUT
} discord_close_reason_t;

typedef enum {
    DISCORD_CLIENT_STATE_ERROR = -2,
    DISCORD_CLIENT_STATE_DISCONNECTED = -1,
    DISCORD_CLIENT_STATE_UNKNOWN,
    DISCORD_CLIENT_STATE_INIT,
    DISCORD_CLIENT_STATE_CONNECTING,
    DISCORD_CLIENT_STATE_CONNECTED
} discord_client_state_t;

enum {
    DISCORD_CLIENT_STATUS_BIT_BUFFER_READY = (1 << 0)
};

typedef struct {
    bool running;
    int interval;
    uint64_t tick_ms;
    bool received_ack;
} discord_heartbeater_t;

typedef esp_err_t(*discord_event_handler_t)(discord_client_handle_t client, discord_event_id_t event, discord_event_data_ptr_t data_ptr);

struct discord_client {
    discord_client_state_t state;
    TaskHandle_t task_handle;
    SemaphoreHandle_t lock;
    EventGroupHandle_t status_bits;
    esp_event_loop_handle_t event_handle;
    discord_event_handler_t event_handler;
    discord_client_config_t* config;
    bool running;
    esp_websocket_client_handle_t ws;
    discord_heartbeater_t heartbeater;
    discord_gateway_session_t* session;
    int last_sequence_number;
    discord_close_reason_t close_reason;
    char* buffer;
    int buffer_len;
};

uint64_t dc_tick_ms();
char* _dc_strcat(const char* str, ...);


#ifdef __cplusplus
}
#endif