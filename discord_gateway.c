#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
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
    int last_sequence_number;
};

static esp_err_t gw_reset(discord_gateway_handle_t gateway);

/**
 * @brief Send payload (serialized to json) to gateway. Payload will be automatically freed
 */
static esp_err_t gw_send(discord_gateway_handle_t gateway, discord_gateway_payload_t* payload);

static void gw_heartbeat_timer_callback(void* arg);
static esp_err_t gw_heartbeat_init(discord_gateway_handle_t gateway);
static esp_err_t gw_heartbeat_start(discord_gateway_handle_t gateway, discord_gateway_hello_t* hello);
static esp_err_t gw_heartbeat_stop(discord_gateway_handle_t gateway);

static esp_err_t gw_identify(discord_gateway_handle_t gateway) {
    return gw_send(gateway, discord_model_gateway_payload(
        DISCORD_OP_IDENTIFY,
        discord_model_gateway_identify(
            gateway->bot.token,
            gateway->bot.intents,
            discord_model_gateway_identify_properties(
                "freertos",
                "esp-idf",
                "esp32"
            )
        )
    ));
}

/**
 * @brief Check event name in payload and invoke appropriate functions
 */
static esp_err_t gw_dispatch(discord_gateway_handle_t gateway, discord_gateway_payload_t* payload) {
    if(DISCORD_GATEWAY_EVENT_READY == payload->t) {
        if(gateway->session != NULL) {
            discord_model_gateway_session_free(gateway->session);
        }

        gateway->session = (discord_gateway_session_t*) payload->d;

        // Detach pointer in order to prevent session deallocation by payload free function
        payload->d = NULL;

        gateway->state = DISCORD_GATEWAY_STATE_READY;
        
        ESP_LOGD(TAG, "Identified [%s#%s (%s), session: %s]", 
            gateway->session->user->username,
            gateway->session->user->discriminator,
            gateway->session->user->id,
            gateway->session->session_id
        );
    } else if(DISCORD_GATEWAY_EVENT_MESSAGE_CREATE == payload->t) {
        discord_message_t* msg = (discord_message_t*) payload->d;

        ESP_LOGD(TAG, "New message from %s#%s: %s",
            msg->author->username,
            msg->author->discriminator,
            msg->content
        );
    } else {
        ESP_LOGW(TAG, "Ignored dispatch event");
    }

    return ESP_OK;
}

static esp_err_t gw_handle_websocket_data(discord_gateway_handle_t gateway, esp_websocket_event_data_t* data) {
    discord_gateway_payload_t* payload = discord_model_gateway_payload_deserialize(data->data_ptr);

    if(payload->s != DISCORD_NULL_SEQUENCE_NUMBER) {
        gateway->last_sequence_number = payload->s;
    }

    ESP_LOGD(TAG, "Received payload (op: %d)", payload->op);

    switch (payload->op) {
        case DISCORD_OP_HELLO:
            gw_heartbeat_start(gateway, (discord_gateway_hello_t*) payload->d);
            discord_model_gateway_payload_free(payload);
            payload = NULL;
            gw_identify(gateway);
            break;
        
        // TODO:
        // After heartbeat is sent, server need to response with OP 11 (heartbeat ACK)
        // If a client does not receive a heartbeat ack between its attempts at sending heartbeats, 
        // it should immediately terminate the connection with a non-1000 close code,
        // reconnect, and attempt to resume.
        //case DISCORD_OP_HEARTBEAT_ACK:
        //    break;

        case DISCORD_OP_DISPATCH:
            gw_dispatch(gateway, payload);
            break;
        
        default:
            ESP_LOGW(TAG, "Unhandled payload with OP code %d", payload->op);
            break;
    }

    if(payload != NULL) {
        discord_model_gateway_payload_free(payload);
    }

    return ESP_OK;
}

static void gw_websocket_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
    discord_gateway_handle_t gateway = (discord_gateway_handle_t) handler_args;
    esp_websocket_event_data_t* data = (esp_websocket_event_data_t*) event_data;

    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGD(TAG, "WEBSOCKET_EVENT_CONNECTED");
            gateway->state = DISCORD_GATEWAY_STATE_INIT;
            break;

        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
            gw_reset(gateway);
            break;

        case WEBSOCKET_EVENT_DATA:
            if(data->op_code == 1) {
                ESP_LOGD(TAG, "Received (payload_len=%d, data_len=%d, payload_offset=%d):\n%.*s", 
                    data->payload_len, 
                    data->data_len, 
                    data->payload_offset,
                    data->data_len,
                    data->data_ptr
                );

                if(data->payload_offset > 0 || data->payload_len > 1024) {
                    ESP_LOGW(TAG, "Payload too big. Buffering not implemented. Parsing skipped.");
                    break;
                }

                gw_handle_websocket_data(gateway, data);
            }
            break;

        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGD(TAG, "WEBSOCKET_EVENT_ERROR");
            gw_reset(gateway);
            gateway->state = DISCORD_GATEWAY_STATE_ERROR;
            break;

        case WEBSOCKET_EVENT_CLOSED:
            ESP_LOGD(TAG, "WEBSOCKET_EVENT_CLOSED");
            gw_reset(gateway);
            break;
            
        default:
            ESP_LOGW(TAG, "WEBSOCKET_EVENT_UNKNOWN %d", event_id);
            break;
    }
}

discord_gateway_handle_t discord_gw_init(const discord_gateway_config_t* config) {
    ESP_LOGD(TAG, "Init");

    discord_gateway_handle_t gateway = calloc(1, sizeof(struct discord_gateway));

    gateway->bot = config->bot;
    ESP_ERROR_CHECK(gw_heartbeat_init(gateway));
    gw_reset(gateway);

    return gateway;
}

esp_err_t discord_gw_open(discord_gateway_handle_t gateway) {
    ESP_LOGD(TAG, "Open");

    if(gateway == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if(gateway->state >= DISCORD_GATEWAY_STATE_INIT) {
        ESP_LOGE(TAG, "Gateway is already open");
        return ESP_FAIL;
    }

    esp_websocket_client_config_t ws_cfg = {
        .uri = "wss://gateway.discord.gg/?v=8&encoding=json"
    };

    gateway->ws = esp_websocket_client_init(&ws_cfg);

    ESP_ERROR_CHECK(esp_websocket_register_events(gateway->ws, WEBSOCKET_EVENT_ANY, gw_websocket_event_handler, (void*) gateway));
    ESP_ERROR_CHECK(esp_websocket_client_start(gateway->ws));

    return ESP_OK;
}

esp_err_t discord_gw_destroy(discord_gateway_handle_t gateway) {
    ESP_LOGD(TAG, "Destroy");

    gw_heartbeat_stop(gateway);
    esp_timer_delete(gateway->heartbeat_timer);
    gateway->heartbeat_timer = NULL;

    esp_websocket_client_destroy(gateway->ws);
    gateway->ws = NULL;

    discord_model_gateway_session_free(gateway->session);
    gateway->session = NULL;

    gw_reset(gateway);

    free(gateway);

    return ESP_OK;
}

static esp_err_t gw_reset(discord_gateway_handle_t gateway) {
    ESP_LOGD(TAG, "Reset");

    gw_heartbeat_stop(gateway);
    gateway->state = DISCORD_GATEWAY_STATE_UNKNOWN;
    gateway->last_sequence_number = DISCORD_NULL_SEQUENCE_NUMBER;

    return ESP_OK;
}

static esp_err_t gw_send(discord_gateway_handle_t gateway, discord_gateway_payload_t* payload) {
    char* payload_raw = discord_model_gateway_payload_serialize(payload);

    ESP_LOGD(TAG, "Sending payload:\n%s", payload_raw);

    esp_websocket_client_send_text(gateway->ws, payload_raw, strlen(payload_raw), portMAX_DELAY);
    free(payload_raw);

    return ESP_OK;
}

// ========== Heartbeat

static void gw_heartbeat_timer_callback(void* arg) {
    ESP_LOGD(TAG, "Heartbeat");

    discord_gateway_handle_t gateway = (discord_gateway_handle_t) arg;
    int s = gateway->last_sequence_number;

    gw_send(gateway, discord_model_gateway_payload(
        DISCORD_OP_HEARTBEAT,
        (discord_gateway_heartbeat_t*) &s
    ));
}

static esp_err_t gw_heartbeat_init(discord_gateway_handle_t gateway) {
    const esp_timer_create_args_t timer_args = {
        .callback = &gw_heartbeat_timer_callback,
        .arg = (void*) gateway
    };

    return esp_timer_create(&timer_args, &(gateway->heartbeat_timer));
}

static esp_err_t gw_heartbeat_start(discord_gateway_handle_t gateway, discord_gateway_hello_t* hello) {
    return esp_timer_start_periodic(gateway->heartbeat_timer, hello->heartbeat_interval * 1000);
}

static esp_err_t gw_heartbeat_stop(discord_gateway_handle_t gateway) {
    return esp_timer_stop(gateway->heartbeat_timer);
}