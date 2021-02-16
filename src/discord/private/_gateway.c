#include "discord/private/_gateway.h"
#include "discord/models.h"
#include "esp_transport_ws.h"

DISCORD_LOG_DEFINE_BASE();

esp_err_t gw_init(discord_client_handle_t client) {
    DISCORD_LOG_FOO();

    ESP_ERROR_CHECK(gw_heartbeat_init(client));
    gw_reset(client);
    client->state = DISCORD_CLIENT_STATE_UNKNOWN;
    client->close_reason = DISCORD_CLOSE_REASON_NOT_REQUESTED;

    return ESP_OK;
}

esp_err_t gw_reset(discord_client_handle_t client) {
    DISCORD_LOG_FOO();
    
    gw_heartbeat_stop(client);
    client->last_sequence_number = DISCORD_NULL_SEQUENCE_NUMBER;
    xEventGroupClearBits(client->status_bits, DISCORD_CLIENT_STATUS_BIT_BUFFER_READY);
    client->buffer_len = 0;

    return ESP_OK;
}

/**
 * @brief Send payload (serialized to json) to gateway. Payload will be automatically freed
 */
esp_err_t gw_send(discord_client_handle_t client, discord_gateway_payload_t* payload) {
    DISCORD_LOG_FOO();

    char* payload_raw = discord_model_gateway_payload_serialize(payload);

    DISCORD_LOGD("%s", payload_raw);

    esp_websocket_client_send_text(client->ws, payload_raw, strlen(payload_raw), portMAX_DELAY);
    free(payload_raw);

    return ESP_OK;
}

esp_err_t gw_open(discord_client_handle_t client) {
    if(client == NULL)
        return ESP_ERR_INVALID_ARG;
    
    DISCORD_LOG_FOO();

    if(gw_is_open(client)) {
        DISCORD_LOGD("Already open");
        return ESP_OK;
    }

    esp_websocket_client_config_t ws_cfg = {
        .uri = "wss://gateway.discord.gg/?v=8&encoding=json",
        .buffer_size = DISCORD_WS_BUFFER_SIZE
    };

    client->ws = esp_websocket_client_init(&ws_cfg);

    ESP_ERROR_CHECK(esp_websocket_register_events(client->ws, WEBSOCKET_EVENT_ANY, gw_websocket_event_handler, (void*) client));
    ESP_ERROR_CHECK(gw_start(client));

    return ESP_OK;
}

bool gw_is_open(discord_client_handle_t client) {
    return client->running && client->state >= DISCORD_CLIENT_STATE_INIT;
}

esp_err_t gw_start(discord_client_handle_t client) {
    DISCORD_LOG_FOO();

    if(gw_is_open(client)) {
        DISCORD_LOGD("Already started");
        return ESP_OK;
    }
    
    client->state = DISCORD_CLIENT_STATE_INIT;

    return esp_websocket_client_start(client->ws);
}

esp_err_t gw_close(discord_client_handle_t client, discord_close_reason_t reason) {
    DISCORD_LOG_FOO();

    if(! gw_is_open(client)) {
        DISCORD_LOGD("Already closed");
        return ESP_OK;
    }

    // do not set client status in this function
    // it will be automatically set in discord task
    
    client->close_reason = reason;

    if(esp_websocket_client_is_connected(client->ws)) {
        esp_websocket_client_close(client->ws, portMAX_DELAY);
    }

    gw_reset(client);

    return ESP_OK;
}

esp_err_t gw_heartbeat_start(discord_client_handle_t client, discord_gateway_hello_t* hello) {
    if(client->heartbeater.running)
        return ESP_OK;
    
    DISCORD_LOG_FOO();
    
    // Set ack to true to prevent first ack checking
    client->heartbeater.received_ack = true;
    client->heartbeater.interval = hello->heartbeat_interval;
    client->heartbeater.tick_ms = dc_tick_ms();
    client->heartbeater.running = true;

    return ESP_OK;
}

esp_err_t gw_heartbeat_send_if_expired(discord_client_handle_t client) {
    if(client->heartbeater.running && dc_tick_ms() - client->heartbeater.tick_ms > client->heartbeater.interval) {
        DISCORD_LOGD("Heartbeat");

        client->heartbeater.tick_ms = dc_tick_ms();

        if(! client->heartbeater.received_ack) {
            DISCORD_LOGW("ACK has not been received since the last heartbeat. Reconnection will follow using IDENTIFY (RESUME is not implemented yet)");
            return gw_close(client, DISCORD_CLOSE_REASON_HEARTBEAT_ACK_NOT_RECEIVED);
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

esp_err_t gw_heartbeat_stop(discord_client_handle_t client) {
    DISCORD_LOG_FOO();

    client->heartbeater.running = false;
    client->heartbeater.interval = 0;
    client->heartbeater.tick_ms = 0;
    client->heartbeater.received_ack = false;

    return ESP_OK;
}

esp_err_t gw_buffer_websocket_data(discord_client_handle_t client, esp_websocket_event_data_t* data) {
    DISCORD_LOG_FOO();

    if(data->payload_len > client->config->buffer_size) {
        DISCORD_LOGW("Payload too big. Wider buffer required.");
        return ESP_FAIL;
    }
    
    DISCORD_LOGD("Buffering received data:\n%.*s", data->data_len, data->data_ptr);
    
    DC_LOCK_ESP_ERR(
        memcpy(client->buffer + data->payload_offset, data->data_ptr, data->data_len);

        if((client->buffer_len = data->data_len + data->payload_offset) >= data->payload_len) {
            DISCORD_LOGD("Buffering done.");
            xEventGroupSetBits(client->status_bits, DISCORD_CLIENT_STATUS_BIT_BUFFER_READY);
        }
    );

    return ESP_OK;
}

esp_err_t gw_handle_buffered_data(discord_client_handle_t client) {
    DISCORD_LOG_FOO();

    discord_gateway_payload_t* payload;

    DC_LOCK_ESP_ERR(payload = discord_model_gateway_payload_deserialize(client->buffer, client->buffer_len));
    
    if(payload == NULL) {
        DISCORD_LOGE("Cannot deserialize payload");
        return ESP_FAIL;
    }

    if(payload->s != DISCORD_NULL_SEQUENCE_NUMBER) {
        client->last_sequence_number = payload->s;
    }

    DISCORD_LOGD("Received payload (op: %d)", payload->op);

    switch (payload->op) {
        case DISCORD_OP_HELLO:
            gw_heartbeat_start(client, (discord_gateway_hello_t*) payload->d);
            discord_model_gateway_payload_free(payload);
            payload = NULL;
            gw_identify(client);
            break;
        
        case DISCORD_OP_HEARTBEAT_ACK:
            DISCORD_LOGD("Heartbeat ack received");
            client->heartbeater.received_ack = true;
            break;

        case DISCORD_OP_DISPATCH:
            gw_dispatch(client, payload);
            break;
        
        default:
            DISCORD_LOGW("Unhandled payload (op: %d)", payload->op);
            break;
    }

    if(payload != NULL) {
        discord_model_gateway_payload_free(payload);
    }

    return ESP_OK;
}

void gw_websocket_event_handler(void* handler_arg, esp_event_base_t base, int32_t event_id, void* event_data) {
    discord_client_handle_t client = (discord_client_handle_t) handler_arg;
    esp_websocket_event_data_t* data = (esp_websocket_event_data_t*) event_data;

    if(data->op_code == WS_TRANSPORT_OPCODES_PONG) { // ignore PONG frame
        return;
    }

    DISCORD_LOGD("Received WebSocket frame (op_code=%d, payload_len=%d, data_len=%d, payload_offset=%d)",
        data->op_code,
        data->payload_len, 
        data->data_len, 
        data->payload_offset
    );

    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            DISCORD_LOGD("WEBSOCKET_EVENT_CONNECTED");
            client->state = DISCORD_CLIENT_STATE_CONNECTING;
            break;

        case WEBSOCKET_EVENT_DATA:
            if(data->op_code == WS_TRANSPORT_OPCODES_TEXT) {
                DC_LOCK_NO_ERR(gw_buffer_websocket_data(client, data));
            }
            break;
        
        case WEBSOCKET_EVENT_ERROR:
            DISCORD_LOGD("WEBSOCKET_EVENT_ERROR");
            client->state = DISCORD_CLIENT_STATE_ERROR;
            break;

        case WEBSOCKET_EVENT_DISCONNECTED:
            DISCORD_LOGD("WEBSOCKET_EVENT_DISCONNECTED");
            client->state = DISCORD_CLIENT_STATE_DISCONNECTED;
            break;

        case WEBSOCKET_EVENT_CLOSED:
            DISCORD_LOGD("WEBSOCKET_EVENT_CLOSED");
            client->state = DISCORD_CLIENT_STATE_DISCONNECTED;
            break;
            
        default:
            DISCORD_LOGW("WEBSOCKET_EVENT_UNKNOWN %d", event_id);
            break;
    }
}

esp_err_t gw_identify(discord_client_handle_t client) {
    DISCORD_LOG_FOO();

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

/**
 * @brief Check event name in payload and invoke appropriate functions
 */
esp_err_t gw_dispatch(discord_client_handle_t client, discord_gateway_payload_t* payload) {
    DISCORD_LOG_FOO();
    
    if(DISCORD_GATEWAY_EVENT_READY == payload->t) {
        if(client->session != NULL) {
            discord_model_gateway_session_free(client->session);
        }

        client->session = (discord_gateway_session_t*) payload->d;

        // Detach pointer in order to prevent session deallocation by payload free function
        payload->d = NULL;

        client->state = DISCORD_CLIENT_STATE_CONNECTED;
        
        DISCORD_LOGD("Identified [%s#%s (%s), session: %s]", 
            client->session->user->username,
            client->session->user->discriminator,
            client->session->user->id,
            client->session->session_id
        );

        DISCORD_EVENT_EMIT(DISCORD_EVENT_CONNECTED, NULL);
    } else if(DISCORD_GATEWAY_EVENT_MESSAGE_CREATE == payload->t) {
        discord_message_t* msg = (discord_message_t*) payload->d;

        DISCORD_LOGD("New message (from %s#%s): %s",
            msg->author->username,
            msg->author->discriminator,
            msg->content
        );

        DISCORD_EVENT_EMIT(DISCORD_EVENT_MESSAGE_RECEIVED, msg);
    } else {
        DISCORD_LOGW("Ignored dispatch event");
    }

    return ESP_OK;
}