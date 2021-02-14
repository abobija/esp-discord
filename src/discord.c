#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_websocket_client.h"
#include "esp_transport_ws.h"
#include "private/discord_models_private.h"
#include "discord_models.h"
#include "discord.h"

#define DC_LOCK(CODE) {\
    if (xSemaphoreTakeRecursive(client->lock, portMAX_DELAY) != pdPASS) {\
        ESP_LOGE(TAG, "Could not lock");\
        return ESP_FAIL;\
    }\
    CODE\
    xSemaphoreGiveRecursive(client->lock);\
}

static const char* TAG = DISCORD_LOG_TAG;

ESP_EVENT_DEFINE_BASE(DISCORD_EVENTS);

typedef enum {
    DISCORD_CLOSE_REASON_NOT_REQUESTED = -1,
    DISCORD_CLOSE_REASON_RECONNECT,
    DISCORD_CLOSE_REASON_LOGOUT
} discord_close_reason_t;

typedef enum {
    DISCORD_CLIENT_STATE_ERROR = -2,
    DISCORD_CLIENT_STATE_DISCONNECTED = -1,
    DISCORD_CLIENT_STATE_UNKNOWN,
    DISCORD_CLIENT_STATE_INIT,
    DISCORD_CLIENT_STATE_CONNECTED
} discord_client_state_t;

typedef struct {
    bool running;
    int interval;
    uint64_t tick_ms;
    bool received_ack;
} discord_heartbeater_t;

struct discord_client {
    discord_client_state_t state;
    TaskHandle_t task_handle;
    SemaphoreHandle_t lock;
    esp_event_loop_handle_t event_handle;
    discord_client_config_t* config;
    bool running;
    esp_websocket_client_handle_t ws;
    discord_heartbeater_t heartbeater;
    discord_gateway_session_t* session;
    int last_sequence_number;
    discord_close_reason_t close_reason;
};

static uint64_t dc_tick_ms(void) {
    return esp_timer_get_time() / 1000;
}

static esp_err_t dc_dispatch_event(discord_client_handle_t client, discord_event_id_t event, discord_event_data_ptr_t data_ptr);

static esp_err_t gw_reset(discord_client_handle_t client);

/**
 * @brief Send payload (serialized to json) to gateway. Payload will be automatically freed
 */
static esp_err_t gw_send(discord_client_handle_t client, discord_gateway_payload_t* payload);

static esp_err_t gw_heartbeat_send_if_expired(discord_client_handle_t client);
#define gw_heartbeat_init(client) gw_heartbeat_stop(client)
static esp_err_t gw_heartbeat_start(discord_client_handle_t client, discord_gateway_hello_t* hello);
static esp_err_t gw_heartbeat_stop(discord_client_handle_t client);

/**
 * @brief Check event name in payload and invoke appropriate functions
 */
static esp_err_t gw_dispatch(discord_client_handle_t client, discord_gateway_payload_t* payload) {
    if(DISCORD_GATEWAY_EVENT_READY == payload->t) {
        if(client->session != NULL) {
            discord_model_gateway_session_free(client->session);
        }

        client->session = (discord_gateway_session_t*) payload->d;

        // Detach pointer in order to prevent session deallocation by payload free function
        payload->d = NULL;

        client->state = DISCORD_CLIENT_STATE_CONNECTED;
        
        ESP_LOGD(TAG, "GW: Identified [%s#%s (%s), session: %s]", 
            client->session->user->username,
            client->session->user->discriminator,
            client->session->user->id,
            client->session->session_id
        );

        dc_dispatch_event(client, DISCORD_EVENT_CONNECTED, NULL);
    } else if(DISCORD_GATEWAY_EVENT_MESSAGE_CREATE == payload->t) {
        discord_message_t* msg = (discord_message_t*) payload->d;

        ESP_LOGD(TAG, "GW: New message (from %s#%s): %s",
            msg->author->username,
            msg->author->discriminator,
            msg->content
        );

        dc_dispatch_event(client, DISCORD_EVENT_MESSAGE_RECEIVED, msg);
    } else {
        ESP_LOGW(TAG, "GW: Ignored dispatch event");
    }

    return ESP_OK;
}

static esp_err_t gw_identify(discord_client_handle_t client) {
    return gw_send(client, discord_model_gateway_payload(
        DISCORD_OP_IDENTIFY,
        discord_model_gateway_identify(
            client->config->token,
            client->config->intents,
            discord_model_gateway_identify_properties(
                "freertos",
                "esp-idf",
                "esp32"
            )
        )
    ));
}

static esp_err_t gw_handle_websocket_data(discord_client_handle_t client, esp_websocket_event_data_t* data) {
    if(data->payload_offset > 0 || data->payload_len > 1024) {
        ESP_LOGW(TAG, "GW: Payload too big. Buffering not implemented. Parsing skipped.");
        return ESP_FAIL;
    }
    
    ESP_LOGD(TAG, "GW: Received data:\n%.*s", data->data_len, data->data_ptr);

    discord_gateway_payload_t* payload = discord_model_gateway_payload_deserialize(data->data_ptr);

    if(payload->s != DISCORD_NULL_SEQUENCE_NUMBER) {
        client->last_sequence_number = payload->s;
    }

    ESP_LOGD(TAG, "GW: Received payload (op: %d)", payload->op);

    switch (payload->op) {
        case DISCORD_OP_HELLO:
            gw_heartbeat_start(client, (discord_gateway_hello_t*) payload->d);
            discord_model_gateway_payload_free(payload);
            payload = NULL;
            gw_identify(client);
            break;
        
        case DISCORD_OP_HEARTBEAT_ACK:
            client->heartbeater.received_ack = true;
            break;

        case DISCORD_OP_DISPATCH:
            gw_dispatch(client, payload);
            break;
        
        default:
            ESP_LOGW(TAG, "GW: Unhandled payload (op: %d)", payload->op);
            break;
    }

    if(payload != NULL) {
        discord_model_gateway_payload_free(payload);
    }

    return ESP_OK;
}

static void gw_websocket_event_handler(void* handler_arg, esp_event_base_t base, int32_t event_id, void* event_data) {
    discord_client_handle_t client = (discord_client_handle_t) handler_arg;
    esp_websocket_event_data_t* data = (esp_websocket_event_data_t*) event_data;

    if(data->op_code == WS_TRANSPORT_OPCODES_PONG) { // ignore PONG frame
        return;
    }

    ESP_LOGD(TAG, "GW: Received WebSocket frame (op_code=%d, payload_len=%d, data_len=%d, payload_offset=%d)",
        data->op_code,
        data->payload_len, 
        data->data_len, 
        data->payload_offset
    );

    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGD(TAG, "GW: WEBSOCKET_EVENT_CONNECTED");
            break;

        case WEBSOCKET_EVENT_DATA:
            if(data->op_code == WS_TRANSPORT_OPCODES_TEXT) {
                gw_handle_websocket_data(client, data);
            }
            break;
        
        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGD(TAG, "GW: WEBSOCKET_EVENT_ERROR");
            client->state = DISCORD_CLIENT_STATE_ERROR;
            break;

        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "GW: WEBSOCKET_EVENT_DISCONNECTED");
            client->state = DISCORD_CLIENT_STATE_DISCONNECTED;
            break;

        case WEBSOCKET_EVENT_CLOSED:
            ESP_LOGD(TAG, "GW: WEBSOCKET_EVENT_CLOSED");
            client->state = DISCORD_CLIENT_STATE_DISCONNECTED;
            break;
            
        default:
            ESP_LOGW(TAG, "GW: WEBSOCKET_EVENT_UNKNOWN %d", event_id);
            break;
    }
}

static esp_err_t gw_start(discord_client_handle_t client) {
    ESP_LOGD(TAG, "GW: Start");

    esp_err_t err;

    DC_LOCK(
        client->state = DISCORD_CLIENT_STATE_INIT;
        err = esp_websocket_client_start(client->ws);
    );

    return err;
}

static esp_err_t gw_open(discord_client_handle_t client) {
    if(client == NULL)
        return ESP_ERR_INVALID_ARG;

    ESP_LOGD(TAG, "GW: Open");

    esp_websocket_client_config_t ws_cfg = {
        .uri = "wss://gateway.discord.gg/?v=8&encoding=json"
    };

    client->ws = esp_websocket_client_init(&ws_cfg);

    ESP_ERROR_CHECK(esp_websocket_register_events(client->ws, WEBSOCKET_EVENT_ANY, gw_websocket_event_handler, (void*) client));
    ESP_ERROR_CHECK(gw_start(client));

    return ESP_OK;
}

static esp_err_t gw_close(discord_client_handle_t client, discord_close_reason_t reason) {
    ESP_LOGD(TAG, "GW: Close");

    DC_LOCK(
        client->close_reason = reason;

        if(esp_websocket_client_is_connected(client->ws)) {
            esp_websocket_client_close(client->ws, portMAX_DELAY);
        }
        
        gw_reset(client);

        client->state = DISCORD_CLIENT_STATE_INIT;
    );

    return ESP_OK;
}

static esp_err_t gw_reconnect(discord_client_handle_t client) {
    ESP_LOGD(TAG, "GW: Reconnect");

    gw_close(client, DISCORD_CLOSE_REASON_RECONNECT);
    ESP_ERROR_CHECK(gw_start(client));

    return ESP_OK;
}

static discord_client_config_t* dc_config_copy(const discord_client_config_t* config) {
    discord_client_config_t* clone = calloc(1, sizeof(discord_client_config_t));

    clone->token = strdup(config->token);
    clone->intents = config->intents;

    return clone;
}

static void dc_config_free(discord_client_config_t* config) {
    if(config == NULL)
        return;

    free(config->token);
    free(config);
}

static esp_err_t gw_init(discord_client_handle_t client) {
    ESP_LOGD(TAG, "GW: Init");

    ESP_ERROR_CHECK(gw_heartbeat_init(client));
    gw_reset(client);

    return ESP_OK;
}

static esp_err_t dc_dispatch_event(discord_client_handle_t client, discord_event_id_t event, discord_event_data_ptr_t data_ptr) {
    ESP_LOGD(TAG, "Dispatch esp_event");

    esp_err_t err;

    discord_event_data_t event_data;
    event_data.client = client;
    event_data.ptr = data_ptr;

    if ((err = esp_event_post_to(client->event_handle, DISCORD_EVENTS, event, &event_data, sizeof(discord_event_data_t), portMAX_DELAY)) != ESP_OK) {
        return err;
    }

    return esp_event_loop_run(client->event_handle, 0);
}

static void dc_task(void* pv) {
    discord_client_handle_t client = (discord_client_handle_t) pv;

    client->running = true;

    while(client->running) {
        if (xSemaphoreTakeRecursive(client->lock, portMAX_DELAY) != pdPASS) {
            ESP_LOGE(TAG, "Failed to lock discord tasks, exiting the task...");
            break;
        }

        switch(client->state) {
            case DISCORD_CLIENT_STATE_UNKNOWN:
                // state shouldn't be unknown in this task
                break;

            case DISCORD_CLIENT_STATE_INIT:
                // client trying to connect...
                break;

            case DISCORD_CLIENT_STATE_CONNECTED:
                gw_heartbeat_send_if_expired(client);
                break;

            case DISCORD_CLIENT_STATE_DISCONNECTED:
                if(client->close_reason == DISCORD_CLOSE_REASON_NOT_REQUESTED) {
                    // This event will be invoked when token is invalid as well
                    // correct reason of closing the connection can be found in frame data
                    // (https://discord.com/developers/docs/topics/opcodes-and-status-codes#gateway-gateway-close-event-codes)
                    //
                    // In this moment websocket client does not emit data in this event
                    // (issue reported: https://github.com/espressif/esp-idf/issues/6535)

                    ESP_LOGE(TAG, "Connection closed unexpectedly. Reason cannot be identified in this moment. Maybe your token is invalid?");
                    discord_logout(client);
                } else {
                    gw_reset(client);
                    client->state = DISCORD_CLIENT_STATE_INIT;
                }
                break;
            
            case DISCORD_CLIENT_STATE_ERROR:
                ESP_LOGE(TAG, "Unhandled error occurred. Disconnecting...");
                discord_logout(client);
                break;
        }

        xSemaphoreGiveRecursive(client->lock);

        vTaskDelay(125 / portTICK_PERIOD_MS);
    }

    client->state = DISCORD_CLIENT_STATE_INIT;
    vTaskDelete(NULL);
}

discord_client_handle_t discord_create(const discord_client_config_t* config) {
    ESP_LOGD(TAG, "Init");

    discord_client_handle_t client = calloc(1, sizeof(struct discord_client));
    
    client->state = DISCORD_CLIENT_STATE_UNKNOWN;

    client->lock = xSemaphoreCreateRecursiveMutex();

    esp_event_loop_args_t event_args = {
        .queue_size = 1,
        .task_name = NULL // no task will be created
    };

    if (esp_event_loop_create(&event_args, &client->event_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Error create event handler for discord client");
        free(client);
        return NULL;
    }

    client->config = dc_config_copy(config);

    gw_init(client);
    
    return client;
}

esp_err_t discord_login(discord_client_handle_t client) {
    if(client == NULL)
        return ESP_ERR_INVALID_ARG;
    
    ESP_LOGD(TAG, "Login");

    if(client->state >= DISCORD_CLIENT_STATE_INIT) {
        ESP_LOGE(TAG, "Client is above (or equal to) init state");
        return ESP_FAIL;
    }

    client->state = DISCORD_CLIENT_STATE_INIT;

    if (xTaskCreate(dc_task, "discord_task", 4 * 1024, client, 5, &client->task_handle) != pdTRUE) {
        ESP_LOGE(TAG, "Error create discord task");
        return ESP_FAIL;
    }

    return gw_open(client);
}

esp_err_t discord_register_events(discord_client_handle_t client, discord_event_id_t event, esp_event_handler_t event_handler, void* event_handler_arg) {
    if(client == NULL)
        return ESP_ERR_INVALID_ARG;
    
    ESP_LOGD(TAG, "Register events");
    
    return esp_event_handler_register_with(client->event_handle, DISCORD_EVENTS, event, event_handler, event_handler_arg);
}

esp_err_t discord_logout(discord_client_handle_t client) {
    if(client == NULL)
        return ESP_ERR_INVALID_ARG;

    ESP_LOGD(TAG, "Logout");

    client->running = false;

    gw_close(client, DISCORD_CLOSE_REASON_LOGOUT);

    esp_websocket_client_destroy(client->ws);
    client->ws = NULL;

    if(client->event_handle) {
        esp_event_loop_delete(client->event_handle);
        client->event_handle = NULL;
    }

    discord_model_gateway_session_free(client->session);
    client->session = NULL;

    client->state = DISCORD_CLIENT_STATE_UNKNOWN;

    return ESP_OK;
}

esp_err_t discord_destroy(discord_client_handle_t client) {
    if(client == NULL)
        return ESP_ERR_INVALID_ARG;
    
    ESP_LOGD(TAG, "Destroy");

    if(client->state >= DISCORD_CLIENT_STATE_INIT) {
        discord_logout(client);
    }

    vSemaphoreDelete(client->lock);
    dc_config_free(client->config);
    free(client);

    return ESP_OK;
}

static esp_err_t gw_reset(discord_client_handle_t client) {
    ESP_LOGD(TAG, "GW: Reset");

    gw_heartbeat_stop(client);
    client->last_sequence_number = DISCORD_NULL_SEQUENCE_NUMBER;
    client->close_reason = DISCORD_CLOSE_REASON_NOT_REQUESTED;

    return ESP_OK;
}

static esp_err_t gw_send(discord_client_handle_t client, discord_gateway_payload_t* payload) {
    ESP_LOGD(TAG, "GW: Sending payload:");

    DC_LOCK(
        char* payload_raw = discord_model_gateway_payload_serialize(payload);

        ESP_LOGD(TAG, "GW: %s", payload_raw);

        esp_websocket_client_send_text(client->ws, payload_raw, strlen(payload_raw), portMAX_DELAY);
        free(payload_raw);
    );

    return ESP_OK;
}

// ========== Heartbeat

static esp_err_t gw_heartbeat_send_if_expired(discord_client_handle_t client) {
    if(client->heartbeater.running && dc_tick_ms() - client->heartbeater.tick_ms > client->heartbeater.interval) {
        ESP_LOGD(TAG, "GW: Heartbeat");

        client->heartbeater.tick_ms = dc_tick_ms();

        if(! client->heartbeater.received_ack) {
            ESP_LOGW(TAG, "GW: ACK has not been received since the last heartbeat. Reconnection will follow using IDENTIFY (RESUME is not implemented yet)");
            return gw_reconnect(client);
        }

        client->heartbeater.received_ack = false;
        int s = client->last_sequence_number;

        return gw_send(client, discord_model_gateway_payload(
            DISCORD_OP_HEARTBEAT,
            (discord_gateway_heartbeat_t*) &s
        ));
    }

    return ESP_OK;
}

static esp_err_t gw_heartbeat_start(discord_client_handle_t client, discord_gateway_hello_t* hello) {
    if(client->heartbeater.running)
        return ESP_OK;
    
    // Set ack to true to prevent first ack checking
    client->heartbeater.received_ack = true;
    client->heartbeater.interval = hello->heartbeat_interval;
    client->heartbeater.tick_ms = dc_tick_ms();
    client->heartbeater.running = true;

    return ESP_OK;
}

static esp_err_t gw_heartbeat_stop(discord_client_handle_t client) {
    client->heartbeater.running = false;
    client->heartbeater.interval = 0;
    client->heartbeater.tick_ms = 0;
    client->heartbeater.received_ack = false;

    return ESP_OK;
}