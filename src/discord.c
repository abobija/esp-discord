#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_websocket_client.h"
#include "discord/private/_discord.h"
#include "discord/private/_gateway.h"
#include "discord/models.h"
#include "discord.h"

DISCORD_LOG_DEFINE_BASE();

ESP_EVENT_DEFINE_BASE(DISCORD_EVENTS);

static discord_client_config_t* dc_config_copy(const discord_client_config_t* config) {
    discord_client_config_t* clone = calloc(1, sizeof(discord_client_config_t));

    clone->token = strdup(config->token);
    clone->intents = config->intents;
    clone->buffer_size = config->buffer_size > DISCORD_MIN_BUFFER_SIZE ? config->buffer_size : DISCORD_MIN_BUFFER_SIZE;

    return clone;
}

static void dc_config_free(discord_client_config_t* config) {
    if(config == NULL)
        return;

    free(config->token);
    free(config);
}

static esp_err_t dc_dispatch_event(discord_client_handle_t client, discord_event_id_t event, discord_event_data_ptr_t data_ptr) {
    DISCORD_LOG_FOO();

    esp_err_t err;

    discord_event_data_t event_data;
    event_data.client = client;
    event_data.ptr = data_ptr;

    if ((err = esp_event_post_to(client->event_handle, DISCORD_EVENTS, event, &event_data, sizeof(discord_event_data_t), portMAX_DELAY)) != ESP_OK) {
        return err;
    }

    return esp_event_loop_run(client->event_handle, 0);
}

static void dc_task(void* arg) {
    DISCORD_LOG_FOO();

    discord_client_handle_t client = (discord_client_handle_t) arg;
    bool restart_gw = false;

    while(client->running) {
        if (xSemaphoreTakeRecursive(client->lock, portMAX_DELAY) != pdPASS) {
            DISCORD_LOGE("Failed to lock discord tasks, exiting the task...");
            break;
        }

        switch(client->state) {
            case DISCORD_CLIENT_STATE_UNKNOWN:
                // gateway not started yet
                // this state will be active just a few ticks,
                // because gateway will be started imidiatelly after task is created
                // shortly, this state can be ignored
                break;

            case DISCORD_CLIENT_STATE_INIT:
                // ws_client trying to connect...
                break;

            case DISCORD_CLIENT_STATE_CONNECTING:
                // ws_client connected, but gateway not identified yet
                break;

            case DISCORD_CLIENT_STATE_CONNECTED:
                gw_heartbeat_send_if_expired(client);
                break;

            case DISCORD_CLIENT_STATE_DISCONNECTED:
                if(DISCORD_CLOSE_REASON_NOT_REQUESTED == client->close_reason) {
                    // This event will be invoked when token is invalid as well
                    // correct reason of closing the connection can be found in frame data
                    // (https://discord.com/developers/docs/topics/opcodes-and-status-codes#gateway-gateway-close-event-codes)
                    //
                    // In this moment websocket client does not emit data in this event
                    // (issue reported: https://github.com/espressif/esp-idf/issues/6535)

                    DISCORD_LOGE("Connection closed unexpectedly. Reason cannot be identified in this moment. Maybe your token is invalid?");
                    discord_logout(client);
                } else if(DISCORD_CLOSE_REASON_HEARTBEAT_ACK_NOT_RECEIVED == client->close_reason) {
                    restart_gw = true;
                } else {
                    DISCORD_LOGW("Close requested but not handled");
                    discord_logout(client);
                }

                break;
            
            case DISCORD_CLIENT_STATE_ERROR:
                DISCORD_LOGE("Unhandled error occurred. Disconnecting...");
                discord_logout(client);
                break;
        }

        xSemaphoreGiveRecursive(client->lock);

        if(client->state >= DISCORD_CLIENT_STATE_CONNECTING) {
            EventBits_t bits = xEventGroupWaitBits(client->status_bits, DISCORD_CLIENT_STATUS_BIT_BUFFER_READY, pdTRUE, pdTRUE, 1000 / portTICK_PERIOD_MS); // poll every 1000ms

            if((DISCORD_CLIENT_STATUS_BIT_BUFFER_READY & bits) != 0) {
                gw_handle_buffered_data(client);
            }
        } else if(DISCORD_CLIENT_STATE_DISCONNECTED == client->state && restart_gw) {
            restart_gw = false;
            DISCORD_LOGD("Restarting gateway...");
            gw_start(client);
        } else {
            vTaskDelay(125 / portTICK_PERIOD_MS);
        }
    }

    DISCORD_LOGD("Task exit...");

    client->state = DISCORD_CLIENT_STATE_UNKNOWN;
    vTaskDelete(NULL);
}

discord_client_handle_t discord_create(const discord_client_config_t* config) {
    DISCORD_LOG_FOO();

    discord_client_handle_t client = calloc(1, sizeof(struct discord_client));
    
    client->lock = xSemaphoreCreateRecursiveMutex();
    client->status_bits = xEventGroupCreate();

    esp_event_loop_args_t event_args = {
        .queue_size = 1,
        .task_name = NULL // no task will be created
    };

    if (esp_event_loop_create(&event_args, &client->event_handle) != ESP_OK) {
        DISCORD_LOGE("Cannot create event handler for discord client");
        free(client);
        return NULL;
    }

    client->config = dc_config_copy(config);
    client->buffer = malloc(client->config->buffer_size);
    client->event_handler = &dc_dispatch_event;

    gw_init(client);
    
    return client;
}

esp_err_t discord_login(discord_client_handle_t client) {
    if(client == NULL)
        return ESP_ERR_INVALID_ARG;
    
    DISCORD_LOG_FOO();

    if(client->state >= DISCORD_CLIENT_STATE_INIT) {
        DISCORD_LOGE("Client is above (or equal to) init state");
        return ESP_FAIL;
    }

    client->running = true;

    if (xTaskCreate(dc_task, "discord_task", DISCORD_TASK_STACK_SIZE, client, DISCORD_TASK_PRIORITY, &client->task_handle) != pdTRUE) {
        DISCORD_LOGE("Cannot create discord task");
        return ESP_FAIL;
    }

    return gw_open(client);
}

esp_err_t discord_register_events(discord_client_handle_t client, discord_event_id_t event, esp_event_handler_t event_handler, void* event_handler_arg) {
    if(client == NULL)
        return ESP_ERR_INVALID_ARG;
    
    DISCORD_LOG_FOO();
    
    return esp_event_handler_register_with(client->event_handle, DISCORD_EVENTS, event, event_handler, event_handler_arg);
}

esp_err_t discord_logout(discord_client_handle_t client) {
    if(client == NULL)
        return ESP_ERR_INVALID_ARG;

    DISCORD_LOG_FOO();

    if(! client->running) {
        DISCORD_LOGW("Client has been already disconnected");
        return ESP_OK;
    }

    client->running = false;

    gw_close(client, DISCORD_CLOSE_REASON_LOGOUT);

    esp_websocket_client_destroy(client->ws);
    client->ws = NULL;

    discord_model_gateway_session_free(client->session);
    client->session = NULL;

    return ESP_OK;
}

esp_err_t discord_destroy(discord_client_handle_t client) {
    if(client == NULL)
        return ESP_ERR_INVALID_ARG;
    
    DISCORD_LOG_FOO();

    client->event_handler = NULL;

    if(client->state >= DISCORD_CLIENT_STATE_INIT) {
        discord_logout(client);
    }

    if(client->event_handle) {
        esp_event_loop_delete(client->event_handle);
        client->event_handle = NULL;
    }

    vSemaphoreDelete(client->lock);
    if(client->status_bits) {
        vEventGroupDelete(client->status_bits);
    }
    free(client->buffer);
    dc_config_free(client->config);
    free(client);

    return ESP_OK;
}