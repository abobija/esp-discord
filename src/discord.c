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
#include "cutils.h"

#define _dc_default(val, default) (val > 0 ? val : default)

#define DISCORD_STOPPED_BIT (1 << 0)

DISCORD_LOG_DEFINE_BASE();

ESP_EVENT_DEFINE_BASE(DISCORD_EVENTS);

static discord_config_t* dc_config_copy(const discord_config_t* config) {
    discord_config_t* clone = cu_ctor(discord_config_t,
        .intents = config->intents,
        .gateway_buffer_size = _dc_default(config->gateway_buffer_size, DISCORD_DEFAULT_GW_BUFFER_SIZE),
        .api_buffer_size = _dc_default(config->api_buffer_size, DISCORD_DEFAULT_API_BUFFER_SIZE),
        .api_timeout_ms = _dc_default(config->api_timeout_ms, DISCORD_DEFAULT_API_TIMEOUT_MS),
        .queue_size = _dc_default(config->queue_size, DISCORD_DEFAULT_QUEUE_SIZE),
        .task_stack_size = _dc_default(config->task_stack_size, DISCORD_DEFAULT_TASK_STACK_SIZE),
        .task_priority = _dc_default(config->task_priority, DISCORD_DEFAULT_TASK_PRIORITY)
    );

    // todo: memcheck

    if(config->token) {
        clone->token = strdup(config->token);
        // todo: memcheck
    } else {
#ifdef CONFIG_DISCORD_TOKEN
        clone->token = strdup(CONFIG_DISCORD_TOKEN);
        // todo: memcheck
#endif
    }

    return clone;
}

static void dc_config_free(discord_config_t* config) {
    if(!config)
        return;

    free(config->token);
    free(config);
}

static esp_err_t dc_dispatch_event(discord_handle_t client, discord_event_t event, discord_event_data_ptr_t data_ptr) {
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

static esp_err_t dc_shutdown(discord_handle_t client) {
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

    discord_handle_t client = (discord_handle_t) arg;
    bool restart = false;
    bool is_shutted_down = false;

    xEventGroupClearBits(client->bits, DISCORD_STOPPED_BIT);

    while(client->running) {
        switch(client->state) {
            case DISCORD_STATE_CONNECTED:
                dcgw_heartbeat_send_if_expired(client);
                break;

            case DISCORD_STATE_DISCONNECTED:
                if(DISCORD_CLOSE_REASON_NOT_REQUESTED == client->close_reason) {
                    char* close_desc = NULL;

                    if(client->close_code != DISCORD_CLOSEOP_NO_CODE) {
                        dcgw_get_close_desc(client, &close_desc);
                    }

                    DISCORD_LOGE(
                        "Connection closed (code=%d, desc=%s)",
                        client->close_code,
                        client->close_code == DISCORD_CLOSEOP_NO_CODE ? "NULL" : (close_desc ? close_desc : "NULL")
                    );

                    if(client->close_code == DISCORD_CLOSEOP_AUTHENTICATION_FAILED) {
                        dc_shutdown(client);     // shutdown only on invalid token
                        is_shutted_down = true;
                    } else {
                        restart = true;          // restart in any other case
                        client->close_code = DISCORD_CLOSEOP_NO_CODE;
                    }
                } else if(DISCORD_CLOSE_REASON_HEARTBEAT_ACK_NOT_RECEIVED == client->close_reason) {
                    restart = true;
                } else {
                    DISCORD_LOGW("Disconnection requested but not handled");
                    dc_shutdown(client);
                    is_shutted_down = true;
                }

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
        } else if(client->state <= DISCORD_STATE_DISCONNECTED) {
            dcapi_destroy(client);
            dcgw_close(client, client->state == DISCORD_STATE_ERROR ? DISCORD_CLOSE_REASON_ERROR : client->close_reason); // do not modify reason if no error

            if(restart || client->state == DISCORD_STATE_ERROR) {
                restart = false;
                DISCORD_LOGI("Restarting discord in 10 sec...");
                vTaskDelay(10000 / portTICK_PERIOD_MS);
                DISCORD_EVENT_FIRE(DISCORD_EVENT_RECONNECTING, NULL);
                dcgw_start(client);
            }
        } else {
            vTaskDelay(125 / portTICK_PERIOD_MS);
        }
    }

    if(!is_shutted_down) {
        dc_shutdown(client);
    }
    
    DISCORD_EVENT_FIRE(DISCORD_EVENT_DISCONNECTED, NULL);
    xEventGroupSetBits(client->bits, DISCORD_STOPPED_BIT);
    DISCORD_LOGD("Task exit.");
    vTaskDelete(NULL);
}

discord_handle_t discord_create(const discord_config_t* config) {
    DISCORD_LOG_FOO();

    discord_handle_t client = cu_tctor(discord_handle_t, struct discord,
        .config = dc_config_copy(config)
    );

    // todo: memcheck

    if(!client->config) {
        free(client);
        return NULL;
    }

    if(!client->config->token) {
        DISCORD_LOGE(
            "Fail to create Discord."
            " Token has not been set."
            " Token can be set in menuconfig under Components -> Discord -> Token."
            " Or manually, token can be supplied via config structure."
        );

        discord_destroy(client);
        return NULL;
    }

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

    client->event_handler = &dc_dispatch_event;

    if(dcgw_init(client) != ESP_OK) {
        DISCORD_LOGE("Fail to init gateway");
        discord_destroy(client);
        return NULL;
    }
    
    return client;
}

esp_err_t discord_login(discord_handle_t client) {
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

    if (xTaskCreate(dc_task, "discord_task", client->config->task_stack_size, client, client->config->task_priority, &client->task_handle) != pdTRUE) {
        DISCORD_LOGE("Fail to create task");
        client->running = false;
        return ESP_FAIL;
    }

    return dcgw_open(client);
}

esp_err_t discord_get_state(discord_handle_t client, discord_gateway_state_t* out_state) {
    if(!client || !out_state) {
        return ESP_ERR_INVALID_ARG;
    }

    *out_state = client->state;
    return ESP_OK;
}

esp_err_t discord_get_close_code(discord_handle_t client, discord_close_code_t* out_code) {
    if(!client || !out_code) {
        return ESP_ERR_INVALID_ARG;
    }

    *out_code = client->close_code;
    return ESP_OK;
}

esp_err_t discord_register_events(discord_handle_t client, discord_event_t event, esp_event_handler_t event_handler, void* event_handler_arg) {
    if(!client)
        return ESP_ERR_INVALID_ARG;
    
    DISCORD_LOG_FOO();
    
    return esp_event_handler_register_with(client->event_handle, DISCORD_EVENTS, event, event_handler, event_handler_arg);
}

esp_err_t discord_unregister_events(discord_handle_t client, discord_event_t event, esp_event_handler_t event_handler) {
    if(!client) {
        return ESP_ERR_INVALID_ARG;
    }

    if(!client->event_handle) {
        return ESP_OK;
    }

    return esp_event_handler_unregister_with(client->event_handle, DISCORD_EVENTS, event, event_handler);
}

esp_err_t discord_logout(discord_handle_t client) {
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

esp_err_t discord_destroy(discord_handle_t client) {
    if(! client) {
        return ESP_ERR_INVALID_ARG;
    }
    
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

    discord_ota_destroy(client);

    dc_config_free(client->config);
    client->config = NULL;
    free(client);

    return ESP_OK;
}

uint64_t discord_tick_ms() {
    return esp_timer_get_time() / 1000;
}