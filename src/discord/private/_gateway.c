#include "discord/private/_gateway.h"
#include "discord/private/_json.h"
#include "discord/message.h"
#include "esp_transport_ws.h"
#include "cutils.h"
#include "estr.h"

DISCORD_LOG_DEFINE_BASE();

static void dcgw_heartbeat_stop(discord_handle_t client) {
    DISCORD_LOG_FOO();

    client->heartbeater.running = false;
    client->heartbeater.interval = 0;
    client->heartbeater.tick_ms = 0;
    client->heartbeater.received_ack = false;
}

static bool dcgw_whether_payload_should_go_into_queue(discord_handle_t client, discord_payload_t* payload) {
    if(!payload)
        return false;

    if(payload->op == DISCORD_OP_DISPATCH) {
        if(client->state < DISCORD_STATE_CONNECTED && payload->t != DISCORD_EVENT_READY) {
            DISCORD_LOGW("Ignoring payload because client is not in CONNECTED state and still not receive READY payload");
            return false;
        }

        switch(payload->t) {
            case DISCORD_EVENT_MESSAGE_RECEIVED:
            case DISCORD_EVENT_MESSAGE_UPDATED: {
                    discord_message_t* msg = (discord_message_t*) payload->d;

                    if(!msg ||
                        !msg->author ||
                        !(msg->type == DISCORD_MESSAGE_DEFAULT || msg->type == DISCORD_MESSAGE_REPLY) || // ignore if not default or reply type
                        estr_eq(msg->author->id, client->session->user->id)) { // ignore our messages
                        return false;
                    }
                }
                break;
            
            case DISCORD_EVENT_MESSAGE_REACTION_ADDED:
            case DISCORD_EVENT_MESSAGE_REACTION_REMOVED: {
                    discord_message_reaction_t* react = (discord_message_reaction_t*) payload->d;

                    // ignore our reactions
                    if(!react || !react->emoji || estr_eq(react->user_id, client->session->user->id)) {
                        return false;
                    }
                }
                break;
                
            default:
                break;
        }
    }

    return true;
}

static discord_close_code_t dcgw_get_close_opcode(discord_handle_t client) {
    if(client->state == DISCORD_STATE_DISCONNECTING && client->gw_buffer_len >= 2) {
        int code = (256 * client->gw_buffer[0] + client->gw_buffer[1]);
        return code >= _DISCORD_CLOSEOP_MIN && code <= _DISCORD_CLOSEOP_MAX ? code : DISCORD_CLOSEOP_NO_CODE;
    }

    return DISCORD_CLOSEOP_NO_CODE;
}

static esp_err_t dcgw_buffer_websocket_data(discord_handle_t client, esp_websocket_event_data_t* data) {
    DISCORD_LOG_FOO();

    if(data->payload_len > client->config->gateway_buffer_size) {
        DISCORD_LOGW("Payload too big. Wider buffer required.");
        return ESP_FAIL;
    }
    
    DISCORD_LOGD("Buffering received data:\n%.*s", data->data_len, data->data_ptr);
    
    memcpy(client->gw_buffer + data->payload_offset, data->data_ptr, data->data_len);

    if((client->gw_buffer_len = data->data_len + data->payload_offset) >= data->payload_len) {
        DISCORD_LOGD("Buffering done");

        // append null terminator
        client->gw_buffer[client->gw_buffer_len] = '\0';

        if(data->op_code == WS_TRANSPORT_OPCODES_CLOSE) {
            client->state = DISCORD_STATE_DISCONNECTING;
            client->close_code = dcgw_get_close_opcode(client);

            return ESP_OK;
        }

        discord_payload_t* payload = discord_json_deserialize_(payload, client->gw_buffer, client->gw_buffer_len);

        if(!payload) {
            DISCORD_LOGE("Fail to deserialize payload");
            return ESP_FAIL;
        }

        if(payload->s != DISCORD_NULL_SEQUENCE_NUMBER) {
            client->last_sequence_number = payload->s;
        }
        
        if(! dcgw_whether_payload_should_go_into_queue(client, payload)) {
            DISCORD_LOGD("Payload ignored");
            discord_payload_free(payload);
        } else if(xQueueSend(client->queue, &payload, 5000 / portTICK_PERIOD_MS) != pdPASS) { // 5sec timeout
            DISCORD_LOGW("Fail to queue the payload");
            discord_payload_free(payload);
        }
    }

    return ESP_OK;
}

static void dcgw_websocket_event_handler(void* handler_arg, esp_event_base_t base, int32_t event_id, void* event_data) {
    discord_handle_t client = (discord_handle_t) handler_arg;
    esp_websocket_event_data_t* data = (esp_websocket_event_data_t*) event_data;

    if(data->op_code == WS_TRANSPORT_OPCODES_PONG) { // ignore PONG frame
        return;
    }

    DISCORD_LOGD("ws event (event=%d, op_code=%d, payload_len=%d, data_len=%d, payload_offset=%d)",
        event_id,
        data->op_code,
        data->payload_len, 
        data->data_len, 
        data->payload_offset
    );

    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            client->state = DISCORD_STATE_CONNECTING;
            break;

        case WEBSOCKET_EVENT_DATA:
            if(data->op_code == WS_TRANSPORT_OPCODES_TEXT || data->op_code == WS_TRANSPORT_OPCODES_CLOSE) {
                dcgw_buffer_websocket_data(client, data);
            }
            break;
        
        case WEBSOCKET_EVENT_ERROR:
            client->state = DISCORD_STATE_ERROR;
            break;

        case WEBSOCKET_EVENT_DISCONNECTED:
            client->state = DISCORD_STATE_DISCONNECTED;
            break;

        case WEBSOCKET_EVENT_CLOSED:
            client->state = DISCORD_STATE_DISCONNECTED;
            break;
            
        default:
            DISCORD_LOGW("Unknown ws event %d", event_id);
            break;
    }
}

esp_err_t dcgw_init(discord_handle_t client) {
    DISCORD_LOG_FOO();

    if(client->state >= DISCORD_STATE_INIT) {
        DISCORD_LOGW("Already inited");
        return ESP_OK;
    }
    
    if(!(client->gw_lock = xSemaphoreCreateMutex()) ||
       !(client->queue = xQueueCreate(client->config->queue_size, sizeof(discord_payload_t*)))) {
        DISCORD_LOGE("Fail to create mutex/queue");
        dcgw_destroy(client);
        return ESP_FAIL;
    }

    if(!(client->gw_buffer = malloc(client->config->gateway_buffer_size + 1))) {
        DISCORD_LOGE("Fail to allocate buffer");
        dcgw_destroy(client);
        return ESP_FAIL;
    }

    dcgw_heartbeat_stop(client);
    client->last_sequence_number = DISCORD_NULL_SEQUENCE_NUMBER;
    client->close_reason = DISCORD_CLOSE_REASON_NOT_REQUESTED;
    client->close_code = DISCORD_CLOSEOP_NO_CODE;
    client->gw_buffer_len = 0;
    client->state = DISCORD_STATE_INIT;

#ifndef CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY
    extern const uint8_t gateway_crt[] asm("_binary_gateway_pem_start");
#endif
    
    esp_websocket_client_config_t ws_cfg = {
        .uri = DISCORD_GW_URL,
        .buffer_size = 512,
#ifndef CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY
        .cert_pem = (const char*) gateway_crt,
#endif
        .task_stack = 5 * 1024,
        .disable_auto_reconnect = true
    };

    if(!(client->ws = esp_websocket_client_init(&ws_cfg))) {
        DISCORD_LOGE("Fail to create ws client");
        dcgw_destroy(client);
        return ESP_FAIL;
    }

    if(esp_websocket_register_events(client->ws, WEBSOCKET_EVENT_ANY, dcgw_websocket_event_handler, (void*) client) != ESP_OK) {
        DISCORD_LOGE("Fail to register ws handler");
        dcgw_destroy(client);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t dcgw_send(discord_handle_t client, discord_payload_t* payload) {
    DISCORD_LOG_FOO();

    if(xSemaphoreTake(client->gw_lock, 5000 / portTICK_PERIOD_MS) != pdTRUE) { // 5sec timeout
        DISCORD_LOGW("Gateway is locked");
        return ESP_FAIL;
    }
    
    char* payload_raw = discord_json_serialize(payload);
    discord_payload_free(payload);

    DISCORD_LOGD("%s", payload_raw);

    int sent_bytes = esp_websocket_client_send_text(client->ws, payload_raw, strlen(payload_raw), 5000 / portTICK_PERIOD_MS); // 5sec timeout
    free(payload_raw);

    if(sent_bytes == ESP_FAIL) {
        DISCORD_LOGW("Fail to send data to gateway");
        client->state = DISCORD_STATE_ERROR;
        xSemaphoreGive(client->gw_lock);
        return ESP_FAIL;
    }
    
    xSemaphoreGive(client->gw_lock);

    return ESP_OK;
}

esp_err_t dcgw_get_close_desc(discord_handle_t client, char** out_description) {
    if(! client || ! out_description) {
        return ESP_ERR_INVALID_ARG;
    }

    *out_description = client->close_code != DISCORD_CLOSEOP_NO_CODE 
        ? client->gw_buffer + 2 : NULL;
    
    return ESP_OK;
}

bool dcgw_is_open(discord_handle_t client) {
    return client && client->state >= DISCORD_STATE_OPEN;
}

esp_err_t dcgw_open(discord_handle_t client) {
    if(client == NULL)
        return ESP_ERR_INVALID_ARG;
    
    DISCORD_LOG_FOO();

    if(dcgw_is_open(client)) {
        DISCORD_LOGD("Already open");
        return ESP_OK;
    }

    return dcgw_start(client);
}

esp_err_t dcgw_start(discord_handle_t client) {
    DISCORD_LOG_FOO();

    if(dcgw_is_open(client)) {
        DISCORD_LOGD("Already started");
        return ESP_OK;
    }
    
    client->close_reason = DISCORD_CLOSE_REASON_NOT_REQUESTED;
    esp_err_t err = esp_websocket_client_start(client->ws);
    client->state = err == ESP_OK ? DISCORD_STATE_OPEN : DISCORD_STATE_ERROR;
    
    return err;
}

esp_err_t dcgw_close(discord_handle_t client, discord_gateway_close_reason_t reason) {
    DISCORD_LOG_FOO();

    if(! client) {
        return ESP_ERR_INVALID_ARG;
    }

    // do not set client status in this function
    // it will be automatically set in ws task

    if(client->gw_lock) { xSemaphoreTake(client->gw_lock, portMAX_DELAY); } // wait to unlock
    client->close_reason = reason;
    dcgw_heartbeat_stop(client);
    client->last_sequence_number = DISCORD_NULL_SEQUENCE_NUMBER;
    
    if(esp_websocket_client_is_connected(client->ws)) {
        esp_websocket_client_close(client->ws, portMAX_DELAY);
    }

    client->gw_buffer_len = 0;
    dcgw_queue_flush(client);
    if(client->gw_lock) { xSemaphoreGive(client->gw_lock); }

    return ESP_OK;
}

esp_err_t dcgw_destroy(discord_handle_t client) {
    DISCORD_LOG_FOO();

    if(! client) {
        return ESP_ERR_INVALID_ARG;
    }

    dcgw_close(client, DISCORD_CLOSE_REASON_DESTROY);
    esp_websocket_client_destroy(client->ws);
    client->ws = NULL;
    free(client->gw_buffer);
    client->gw_buffer = NULL;

    if(client->gw_lock) {
        xSemaphoreTake(client->gw_lock, portMAX_DELAY); // wait to unlock
        vSemaphoreDelete(client->gw_lock);
        client->gw_lock = NULL;
    }

    if(client->queue) {
        dcgw_queue_flush(client);
        vQueueDelete(client->queue);
        client->queue = NULL;
    }

    client->state = DISCORD_STATE_UNKNOWN;

    return ESP_OK;
}

esp_err_t dcgw_queue_flush(discord_handle_t client) {
    if(!client || !client->queue) {
        return ESP_ERR_INVALID_ARG;
    }
    
    discord_payload_t* payload = NULL;

    while(xQueueReceive(client->queue, &payload, (TickType_t) 0) == pdPASS) {
        discord_payload_free(payload);
    }

    return ESP_OK;
}

static esp_err_t dcgw_heartbeat_start(discord_handle_t client, discord_hello_t* hello) {
    if(client->heartbeater.running)
        return ESP_OK;
    
    DISCORD_LOG_FOO();
    
    client->heartbeater.received_ack = true; // True to prevent first ack checking
    client->heartbeater.interval = hello->heartbeat_interval;
    client->heartbeater.tick_ms = discord_tick_ms();
    client->heartbeater.running = true;

    return ESP_OK;
}

esp_err_t dcgw_heartbeat_send_if_expired(discord_handle_t client) {
    if(client->heartbeater.running && discord_tick_ms() - client->heartbeater.tick_ms > client->heartbeater.interval) {
        DISCORD_LOGD("Heartbeat");

        client->heartbeater.tick_ms = discord_tick_ms();

        if(!client->heartbeater.received_ack) {
            DISCORD_LOGW("ACK has not been received since the last heartbeat. Reconnection will follow using IDENTIFY (RESUME is not implemented yet)");
            dcgw_close(client, DISCORD_CLOSE_REASON_HEARTBEAT_ACK_NOT_RECEIVED);
            return ESP_ERR_INVALID_STATE;
        }

        client->heartbeater.received_ack = false;
        int s = client->last_sequence_number;

        // todo: memcheck
        return dcgw_send(client, cu_ctor(discord_payload_t,
            .op = DISCORD_OP_HEARTBEAT,
            .d = (discord_heartbeat_t*) &s
        ));
    }

    return ESP_OK;
}

esp_err_t dcgw_identify(discord_handle_t client) {
    DISCORD_LOG_FOO();

    // todo: memchecks
    return dcgw_send(client, cu_ctor(discord_payload_t,
        .op = DISCORD_OP_IDENTIFY,
        .d = cu_ctor(discord_identify_t,
            .token = strdup(client->config->token),
            .intents = client->config->intents,
            .properties = cu_ctor(discord_identify_properties_t,
                .os = estr_cat("esp-idf (", esp_get_idf_version(), ")"),
                .browser = strdup("esp-discord (" DISCORD_VER_STRING ")"),
                .device = strdup(CONFIG_IDF_TARGET)
            )
        )
    ));
}

/**
 * @brief Check event name in payload and invoke appropriate functions
 */
static esp_err_t dcgw_dispatch(discord_handle_t client, discord_payload_t* payload) {
    DISCORD_LOG_FOO();

    if(DISCORD_EVENT_READY == payload->t) {
        if(client->session) {
            discord_session_free(client->session);
        }

        client->session = (discord_session_t*) payload->d;

        // Detach pointer in order to prevent session deallocation by payload free function
        payload->d = NULL;

        client->state = DISCORD_STATE_CONNECTED;
        
        DISCORD_LOGD("Identified [%s#%s (%s), session: %s]", 
            client->session->user->username,
            client->session->user->discriminator,
            client->session->user->id,
            client->session->session_id
        );

        discord_session_t* _s = client->session;

        discord_session_t* session_clone = cu_ctor(discord_session_t,
            .session_id = strdup(_s->session_id),
            .user = cu_ctor(discord_user_t,
                .id = strdup(_s->user->id),
                .bot = _s->user->bot,
                .username = strdup(_s->user->username),
                .discriminator = strdup(_s->user->discriminator)
            )
        );

        // todo: memcheck

        DISCORD_EVENT_FIRE(DISCORD_EVENT_CONNECTED, session_clone);
        discord_session_free(session_clone);

        return ESP_OK;
    }

    if(payload->t > DISCORD_EVENT_CONNECTED) {
        // client is connected. fire the event!
        DISCORD_EVENT_FIRE(payload->t, payload->d);
    }

    return ESP_OK;
}

esp_err_t dcgw_handle_payload(discord_handle_t client, discord_payload_t* payload) {
    DISCORD_LOG_FOO();

    if(!payload)
        return ESP_FAIL;

    DISCORD_LOGD("Received payload (op: %d)", payload->op);

    switch (payload->op) {
        case DISCORD_OP_HELLO:
            dcgw_heartbeat_start(client, (discord_hello_t*) payload->d);
            discord_payload_free(payload);
            payload = NULL;
            dcgw_identify(client);
            break;
        
        case DISCORD_OP_HEARTBEAT_ACK:
            DISCORD_LOGD("Heartbeat ack received");
            client->heartbeater.received_ack = true;
            break;

        case DISCORD_OP_DISPATCH:
            dcgw_dispatch(client, payload);
            break;
        
        default:
            DISCORD_LOGW("Unhandled payload (op: %d)", payload->op);
            break;
    }

    if(payload) {
        discord_payload_free(payload);
    }

    return ESP_OK;
}