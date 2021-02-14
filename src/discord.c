#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_websocket_client.h"
#include "private/discord_models_private.h"
#include "discord_models.h"
#include "discord.h"

static const char* TAG = DISCORD_LOG_TAG;

typedef enum {
    DISCORD_CLOSE_REASON_NOT_REQUESTED = -1,
    DISCORD_CLOSE_REASON_RECONNECT,
    DISCORD_CLOSE_REASON_LOGOUT
} discord_close_reason_t;

struct discord_client {
    esp_event_loop_handle_t event_handle;
    discord_client_config_t* config;
    esp_websocket_client_handle_t ws;
    bool heartbeat_timer_running;
    esp_timer_handle_t heartbeat_timer;
    bool heartbeat_ack_received;
    discord_gateway_session_t* session;
    int last_sequence_number;
    discord_close_reason_t close_reason;
};

ESP_EVENT_DEFINE_BASE(DISCORD_EVENTS);

static esp_err_t dc_dispatch_event(discord_client_handle_t client, discord_event_id_t event, discord_event_data_ptr_t data_ptr);

static esp_err_t gw_reset(discord_client_handle_t client);

/**
 * @brief Send payload (serialized to json) to gateway. Payload will be automatically freed
 */
static esp_err_t gw_send(discord_client_handle_t client, discord_gateway_payload_t* payload);

static void gw_heartbeat_timer_callback(void* arg);
static esp_err_t gw_heartbeat_init(discord_client_handle_t client);
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
            client->heartbeat_ack_received = true;
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

    if(data->op_code == 10) { // ignore PONG frame
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

        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "GW: WEBSOCKET_EVENT_DISCONNECTED");
            gw_reset(client);
            break;

        case WEBSOCKET_EVENT_DATA:
            if(data->op_code == 1) {
                ESP_LOGD(TAG, "GW: Received data:\n%.*s", data->data_len, data->data_ptr);

                if(data->payload_offset > 0 || data->payload_len > 1024) {
                    ESP_LOGW(TAG, "GW: Payload too big. Buffering not implemented. Parsing skipped.");
                    break;
                }

                gw_handle_websocket_data(client, data);
            }
            break;

        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGD(TAG, "GW: WEBSOCKET_EVENT_ERROR");
            gw_reset(client);
            break;

        case WEBSOCKET_EVENT_CLOSED:
            ESP_LOGD(TAG, "GW: WEBSOCKET_EVENT_CLOSED");

            if(client->close_reason == DISCORD_CLOSE_REASON_NOT_REQUESTED) {
                // This event will be invoked when token is invalid as well
                // correct reason of closing the connection can be found in frame data
                // (https://discord.com/developers/docs/topics/opcodes-and-status-codes#gateway-gateway-close-event-codes)
                //
                // In this moment websocket client does not emit data in this event
                // (issue reported: https://github.com/espressif/esp-idf/issues/6535)

                ESP_LOGE(TAG, "Connection closed unexpectedly. Reason cannot be identified in this moment. Maybe your token is invalid?");
                gw_reset(client);
            }
            
            break;
            
        default:
            ESP_LOGW(TAG, "GW: WEBSOCKET_EVENT_UNKNOWN %d", event_id);
            break;
    }
}

static esp_err_t gw_start(discord_client_handle_t client) {
    ESP_LOGD(TAG, "GW: Start");

    return esp_websocket_client_start(client->ws);
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

    client->close_reason = reason;

    if(esp_websocket_client_is_connected(client->ws)) {
        esp_websocket_client_close(client->ws, portMAX_DELAY);
    }
    
    gw_reset(client);

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

discord_client_handle_t discord_create(const discord_client_config_t* config) {
    ESP_LOGD(TAG, "Init");

    discord_client_handle_t client = calloc(1, sizeof(struct discord_client));
    
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

    gw_close(client, DISCORD_CLOSE_REASON_LOGOUT);

    esp_timer_delete(client->heartbeat_timer);
    client->heartbeat_timer = NULL;

    esp_websocket_client_destroy(client->ws);
    client->ws = NULL;

    if(client->event_handle) {
        esp_event_loop_delete(client->event_handle);
        client->event_handle = NULL;
    }

    discord_model_gateway_session_free(client->session);
    client->session = NULL;

    return ESP_OK;
}

esp_err_t discord_destroy(discord_client_handle_t client) {
    if(client == NULL)
        return ESP_ERR_INVALID_ARG;
    
    ESP_LOGD(TAG, "Destroy");

    discord_logout(client);

    dc_config_free(client->config);
    free(client);

    return ESP_OK;
}

static esp_err_t gw_reset(discord_client_handle_t client) {
    ESP_LOGD(TAG, "GW: Reset");

    gw_heartbeat_stop(client);
    client->last_sequence_number = DISCORD_NULL_SEQUENCE_NUMBER;
    client->heartbeat_ack_received = false;
    client->close_reason = DISCORD_CLOSE_REASON_NOT_REQUESTED;

    return ESP_OK;
}

static esp_err_t gw_send(discord_client_handle_t client, discord_gateway_payload_t* payload) {
    char* payload_raw = discord_model_gateway_payload_serialize(payload);

    ESP_LOGD(TAG, "GW: Sending payload:\n%s", payload_raw);

    esp_websocket_client_send_text(client->ws, payload_raw, strlen(payload_raw), portMAX_DELAY);
    free(payload_raw);

    return ESP_OK;
}

// ========== Heartbeat

static void gw_heartbeat_timer_callback(void* arg) {
    ESP_LOGD(TAG, "GW: Heartbeat");

    discord_client_handle_t client = (discord_client_handle_t) arg;

    if(! client->heartbeat_ack_received) {
        ESP_LOGW(TAG, "GW: ACK has not been received since the last heartbeat. Reconnection will follow using IDENTIFY (RESUME is not implemented yet)");
        gw_reconnect(client);
        return;
    }

    client->heartbeat_ack_received = false;
    int s = client->last_sequence_number;

    gw_send(client, discord_model_gateway_payload(
        DISCORD_OP_HEARTBEAT,
        (discord_gateway_heartbeat_t*) &s
    ));
}

static esp_err_t gw_heartbeat_init(discord_client_handle_t client) {
    const esp_timer_create_args_t timer_args = {
        .callback = &gw_heartbeat_timer_callback,
        .arg = (void*) client
    };

    client->heartbeat_ack_received = false;
    client->heartbeat_timer_running = false;

    return esp_timer_create(&timer_args, &(client->heartbeat_timer));
}

static esp_err_t gw_heartbeat_start(discord_client_handle_t client, discord_gateway_hello_t* hello) {
    if(client->heartbeat_timer_running)
        return ESP_OK;

    client->heartbeat_timer_running = true;

    // Set to true to prevent first ack checking in callback
    client->heartbeat_ack_received = true;

    return esp_timer_start_periodic(client->heartbeat_timer, hello->heartbeat_interval * 1000);
}

static esp_err_t gw_heartbeat_stop(discord_client_handle_t client) {
    if(! client->heartbeat_timer_running)
        return ESP_OK;

    client->heartbeat_timer_running = false;
    client->heartbeat_ack_received = false;

    return esp_timer_stop(client->heartbeat_timer);
}