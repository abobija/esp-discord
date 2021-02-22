#include "discord.h"
#include "discord/private/_gateway.h"
#include "discord/private/_api.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_websocket_client.h"

#define DISCORD_STOPPED_BIT (1 << 0)

DISCORD_LOG_DEFINE_BASE();

ESP_EVENT_DEFINE_BASE(DISCORD_EVENTS);

static discord_client_config_t* dc_config_copy(const discord_client_config_t* config) {
    discord_client_config_t* clone = calloc(1, sizeof(discord_client_config_t));

    clone->token = strdup(config->token);
    clone->intents = config->intents;
    clone->buffer_size = config->buffer_size > DISCORD_DEFAULT_BUFFER_SIZE ? config->buffer_size : DISCORD_DEFAULT_BUFFER_SIZE;

    return clone;
}

static void dc_config_free(discord_client_config_t* config) {
    if(!config)
        return;

    free(config->token);
    free(config);
}

static esp_err_t dc_dispatch_event(discord_client_handle_t client, discord_event_t event, discord_event_data_ptr_t data_ptr) {
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

static esp_err_t dc_shutdown(discord_client_handle_t client) {
    if(!client)
        return ESP_ERR_INVALID_ARG;

    DISCORD_LOG_FOO();

    client->running = false;
    dcgw_destroy(client);
    dcapi_destroy(client);
    discord_session_free(client->session);
    client->session = NULL;

    return ESP_OK;
}

static void dc_task(void* arg) {
    DISCORD_LOG_FOO();

    discord_client_handle_t client = (discord_client_handle_t) arg;
    bool restart_gw = false;
    bool is_shutted_down = false;

    xEventGroupClearBits(client->bits, DISCORD_STOPPED_BIT);

    while(client->running) {
        switch(client->state) {
            case DISCORD_STATE_CONNECTED:
                dcgw_heartbeat_send_if_expired(client);
                break;

            case DISCORD_STATE_DISCONNECTED:
                if(DISCORD_CLOSE_REASON_NOT_REQUESTED == client->close_reason) {
                    if(client->close_code == DISCORD_CLOSEOP_NO_CODE) {
                        DISCORD_LOGE("Connection closed with unknown reason");
                    } else {
                        DISCORD_LOGE("Connection closed with code %d: %s", client->close_code, dcgw_get_close_desc(client));
                        client->close_code = DISCORD_CLOSEOP_NO_CODE;
                    }

                    dc_shutdown(client);
                    is_shutted_down = true;
                } else if(DISCORD_CLOSE_REASON_HEARTBEAT_ACK_NOT_RECEIVED == client->close_reason) {
                    restart_gw = true;
                } else {
                    DISCORD_LOGW("Disconnection requested but not handled");
                    dc_shutdown(client);
                    is_shutted_down = true;
                }

                break;
            
            case DISCORD_STATE_ERROR:
                DISCORD_LOGE("Unhandled error occurred. Disconnecting...");
                dc_shutdown(client);
                is_shutted_down = true;
                break;

            default: // ignore other states
                break;
        }

        if(is_shutted_down) {
            break;
        }

        if(client->state >= DISCORD_STATE_CONNECTING) {
            discord_payload_t* payload = NULL;

            if(xQueueReceive(client->queue, &payload, 1000 / portTICK_PERIOD_MS) == pdPASS) { // poll every 1 sec
                dcgw_handle_payload(client, payload);
            }
        } else if(DISCORD_STATE_DISCONNECTED == client->state && restart_gw) {
            restart_gw = false;
            DISCORD_LOGD("Restarting gateway...");
            DISCORD_EVENT_FIRE(DISCORD_EVENT_RECONNECTING, NULL);
            dcgw_start(client);
        } else {
            vTaskDelay(125 / portTICK_PERIOD_MS);
        }
    }

    if(! is_shutted_down) {
        dc_shutdown(client);
    }
    
    DISCORD_EVENT_FIRE(DISCORD_EVENT_DISCONNECTED, NULL);
    xEventGroupSetBits(client->bits, DISCORD_STOPPED_BIT);
    DISCORD_LOGD("Task exit.");
    vTaskDelete(NULL);
}

discord_client_handle_t discord_create(const discord_client_config_t* config) {
    DISCORD_LOG_FOO();

    discord_client_handle_t client = calloc(1, sizeof(struct discord_client));

    esp_event_loop_args_t event_args = {
        .queue_size = 1,
        .task_name = NULL // no task will be created
    };

    if(!(client->bits = xEventGroupCreate())) {
        DISCORD_LOGE("Fail to create bits group");
        discord_destroy(client);
        return NULL;
    }

    xEventGroupSetBits(client->bits, DISCORD_STOPPED_BIT);

    if (esp_event_loop_create(&event_args, &client->event_handle) != ESP_OK) {
        DISCORD_LOGE("Fail to create event handler");
        discord_destroy(client);
        return NULL;
    }

    client->config = dc_config_copy(config);
    client->event_handler = &dc_dispatch_event;

    if(dcgw_init(client) != ESP_OK) {
        DISCORD_LOGE("Fail to init gateway");
        discord_destroy(client);
        return NULL;
    }
    
    return client;
}

esp_err_t discord_login(discord_client_handle_t client) {
    if(!client)
        return ESP_ERR_INVALID_ARG;
    
    DISCORD_LOG_FOO();

    if (xTaskGetCurrentTaskHandle() == client->task_handle) {
        DISCORD_LOGE("Cannot login from event handler");
        return ESP_FAIL;
    }

    if(client->running || dcgw_is_open(client)) {
        DISCORD_LOGE("Already connected (or trying to connect)");
        return ESP_OK;
    }

    if((xEventGroupWaitBits(client->bits, DISCORD_STOPPED_BIT, pdFALSE, pdTRUE, 5000 / portTICK_PERIOD_MS) & DISCORD_STOPPED_BIT) == 0) { // wait for stop
        DISCORD_LOGE("Timeout while waited for disconnection");
        return ESP_FAIL;
    }

    // wait a little just to ensure that previous discord task is exited.
    // in case if discord_login is called from different task, and DISCORD_STOPPED_BIT is just raised
    vTaskDelay(50 / portTICK_PERIOD_MS);

    client->running = true;

    if (xTaskCreate(dc_task, "discord_task", DISCORD_DEFAULT_TASK_STACK_SIZE, client, DISCORD_DEFAULT_TASK_PRIORITY, &client->task_handle) != pdTRUE) {
        DISCORD_LOGE("Fail to create task");
        client->running = false;
        return ESP_FAIL;
    }

    return dcgw_open(client);
}

esp_err_t discord_register_events(discord_client_handle_t client, discord_event_t event, esp_event_handler_t event_handler, void* event_handler_arg) {
    if(!client)
        return ESP_ERR_INVALID_ARG;
    
    DISCORD_LOG_FOO();
    
    return esp_event_handler_register_with(client->event_handle, DISCORD_EVENTS, event, event_handler, event_handler_arg);
}

esp_err_t discord_logout(discord_client_handle_t client) {
    if(!client)
        return ESP_ERR_INVALID_ARG;

    DISCORD_LOG_FOO();

    if (xTaskGetCurrentTaskHandle() == client->task_handle) {
        DISCORD_LOGE("Cannot logout from event handler");
        return ESP_FAIL;
    }

    if(!client->running) {
        DISCORD_LOGW("Not logged in");
        return ESP_OK;
    }

    client->running = false;
    xEventGroupWaitBits(client->bits, DISCORD_STOPPED_BIT, pdFALSE, pdTRUE, portMAX_DELAY); // wait for the discord task to be stopped

    return ESP_OK;
}

esp_err_t discord_destroy(discord_client_handle_t client) {
    if(!client)
        return ESP_ERR_INVALID_ARG;
    
    DISCORD_LOG_FOO();
    
    if (xTaskGetCurrentTaskHandle() == client->task_handle) {
        DISCORD_LOGE("Cannot destroy from event handler");
        return ESP_FAIL;
    }

    discord_logout(client);
    client->event_handler = NULL;

    if(client->event_handle) {
        esp_event_loop_delete(client->event_handle);
        client->event_handle = NULL;
    }

    if(client->bits) {
        vEventGroupDelete(client->bits);
        client->bits = NULL;
    }

    dc_config_free(client->config);
    client->config = NULL;
    free(client);

    return ESP_OK;
}