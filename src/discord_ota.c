#include "esp_ota_ops.h"
#include "discord/private/_discord.h"
#include "discord_ota.h"
#include "discord/session.h"
#include "nvs_flash.h"

DISCORD_LOG_DEFINE_BASE();

#define DCOTA_FOREACH_ERR(ERR)                            \
    ERR(DISCORD_OTA_OK)                                   \
    ERR(DISCORD_OTA_FAIL)                                 \
    ERR(DISCORD_OTA_ERR_SMALL_BUFFER)                     \
    ERR(DISCORD_OTA_ERR_FAIL_TO_READ_RUNNING_FW_DESC)     \
    ERR(DISCORD_OTA_ERR_FAIL_TO_READ_INVALID_FW_DESC)     \
    ERR(DISCORD_OTA_ERR_NEW_VER_SAME_AS_INVALID_VER)      \
    ERR(DISCORD_OTA_ERR_NEW_VER_SAME_AS_RUNNING_VER)      \
    ERR(DISCORD_OTA_ERR_FAIL_TO_BEGIN)                    \
    ERR(DISCORD_OTA_ERR_FAIL_TO_WRITE)                    \
    ERR(DISCORD_OTA_ERR_INVALID_NUM_OF_MSG_ATTACHMENTS)   \
    ERR(DISCORD_OTA_ERR_INVALID_FW_FILE_TYPE)             \
    ERR(DISCORD_OTA_ERR_FAIL_TO_FIND_UPDATE_PARTITION)    \
    ERR(DISCORD_OTA_ERR_NEW_FW_CANNOT_FIT_INTO_PARTITION) \
    ERR(DISCORD_OTA_ERR_FAIL_TO_DOWNLOAD_FW)              \
    ERR(DISCORD_OTA_ERR_IMAGE_CORRUPTED)                  \
    ERR(DISCORD_OTA_ERR_FAIL_TO_END)                      \
    ERR(DISCORD_OTA_ERR_FAIL_TO_MOUNT)                    \
    ERR(DISCORD_OTA_ERR_FAIL_TO_CHECK_ADMIN_PERMISSIONS)  \
    ERR(DISCORD_OTA_ERR_ADMIN_PERMISSIONS_REQUIRED)       \
    ERR(DISCORD_OTA_ERR_INVALID_CHANNEL_CONFIG)           \
    ERR(DISCORD_OTA_ERR_FAIL_TO_FETCH_CHANNELS)           \
    ERR(DISCORD_OTA_ERR_OTA_CHANNEL_NOT_FOUND)            \
    ERR(DISCORD_OTA_ERR_OTA_WRONG_CHANNEL)                \
    ERR(DISCORD_OTA_ERR_INVALID_OTA_MSG_PREFIX_LENGHT)    \
    ERR(DISCORD_OTA_ERR_UNKNOWN_SUBCOMMAND)               \
    ERR(DISCORD_OTA_ERR_FAIL_TO_CONSTRUCT_OTA_STATUS)     \
    ERR(DISCORD_OTA_ERR_INVALID_COMMAND_FORMAT)           \

#define DCOTA_GENERATE_ENUM(ENUM) ENUM,
#define DCOTA_GENERATE_STRING(STRING) #STRING,

typedef enum {
    DCOTA_FOREACH_ERR(DCOTA_GENERATE_ENUM)
} discord_ota_err_t;

static const char* discord_ota_err_string[] = {
    DCOTA_FOREACH_ERR(DCOTA_GENERATE_STRING)
};

// Buffer required for image header
// Once when image is checked buffer can be freed

#define DISCORD_OTA_BUFFER_SIZE 1024

struct discord_ota {
    discord_ota_config_t* config;
    void* buffer;
    int buffer_offset;
    esp_ota_handle_t update_handle;
    const esp_partition_t* update_partition;
    bool image_was_checked;
    discord_ota_err_t error;
    int download_percentage;
    int download_percentage_checkpoint;
};

typedef enum {
    DISCORD_OTA_TOKEN_FROM_CONFIG,
    DISCORD_OTA_TOKEN_FROM_NVS
} discord_ota_token_type;

typedef struct {
    esp_err_t err;
    discord_ota_token_type type;
    char* val;
} discord_ota_token_t;

static discord_ota_token_t discord_ota_get_token();
static esp_err_t discord_ota_connected_handler(discord_handle_t client);
static esp_err_t discord_ota_disconnected_handler(discord_handle_t client);
static esp_err_t discord_ota_perform(discord_handle_t client, discord_message_t* firmware_message);

static void ota_state_reset(discord_handle_t client) {
    discord_ota_handle_t ota = client->ota;

    // reset everything except config

    if(ota->buffer) { free(ota->buffer); }
    ota->buffer_offset = 0;
    ota->update_handle = 0;
    ota->update_partition = NULL;
    ota->image_was_checked = false;
    ota->error = DISCORD_OTA_OK;
    ota->download_percentage = 0;
    ota->download_percentage_checkpoint = 0;
}

static void ota_on_connected(void* handler_arg, esp_event_base_t base, int32_t event_id, void* event_data) {
    discord_ota_connected_handler((discord_handle_t) handler_arg);
}

static void ota_on_message(void* handler_arg, esp_event_base_t base, int32_t event_id, void* event_data) {
    discord_event_data_t* data = (discord_event_data_t*) event_data;
    discord_message_t* msg = (discord_message_t*) data->ptr;
    discord_ota_perform((discord_handle_t) handler_arg, msg);
}

static void ota_on_disconnected(void* handler_arg, esp_event_base_t base, int32_t event_id, void* event_data) {
    discord_ota_disconnected_handler((discord_handle_t) handler_arg);
}

esp_err_t discord_ota_init(discord_handle_t client, discord_ota_config_t* config) {
    if(!client) {
        return ESP_ERR_INVALID_ARG;
    }

    if(client->running || client->state >= DISCORD_STATE_OPEN) {
        DISCORD_LOGE("Fail to init. Initialization of OTA should be done before Discord login");
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_err_t err = ESP_OK;
    discord_ota_handle_t ota = cu_tctor(discord_ota_handle_t, struct discord_ota,
        .config = cu_ctor(discord_ota_config_t)
    );

    client->ota = ota;

    if(config) {
        ota->config->prefix = STRDUP(config->prefix);
        ota->config->multiple_ota = config->multiple_ota;
        ota->config->success_feedback_disabled = config->success_feedback_disabled;
        ota->config->error_feedback_disabled = config->error_feedback_disabled;
        ota->config->administrator_only_disabled = config->administrator_only_disabled;
        if(config->channel) {
            ota->config->channel = cu_ctor(discord_channel_t,
                .id = STRDUP(config->channel->id),
                .name = STRDUP(config->channel->name)
            );
        }
    }

    if((err = discord_register_events(client, DISCORD_EVENT_MESSAGE_RECEIVED, ota_on_message, client)) != ESP_OK) {
        discord_ota_destroy(client);
        return err;
    }

    if(ota->config->multiple_ota) {
        discord_ota_token_t token = discord_ota_get_token();

        if(token.err != ESP_OK) {
            discord_ota_destroy(client);
            return err;
        }

        free(client->config->token);
        client->config->token = token.val;
        token.val = NULL;

        if((err = discord_register_events(client, DISCORD_EVENT_CONNECTED, ota_on_connected, client)) != ESP_OK
            || (err = discord_register_events(client, DISCORD_EVENT_DISCONNECTED, ota_on_disconnected, client)) != ESP_OK) {
            discord_ota_destroy(client);
            return err;
        }
    }

    return err;
}

// Returns true when image header is buffered and image is checked
static bool ota_image(discord_download_info_t* file, discord_ota_handle_t ota_hndl) {
    if(ota_hndl->image_was_checked) { // image is already checked
        return true;
    }

    // This sould not happed but for any case...
    if(ota_hndl->buffer_offset + file->length > DISCORD_OTA_BUFFER_SIZE) {
        ota_hndl->error = DISCORD_OTA_ERR_SMALL_BUFFER;
        return false;
    }

    // Buffering...
    memcpy(ota_hndl->buffer + ota_hndl->buffer_offset, file->data, file->length);
    ota_hndl->buffer_offset += file->length;

    if(ota_hndl->buffer_offset > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) { // Image header is in buffer
        esp_app_desc_t new_app;
        memcpy(&new_app, ota_hndl->buffer + sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t), sizeof(esp_app_desc_t));

        const esp_partition_t *running_partition = esp_ota_get_running_partition();

        esp_app_desc_t running_app;
        if (esp_ota_get_partition_description(running_partition, &running_app) != ESP_OK) {
            ota_hndl->error = DISCORD_OTA_ERR_FAIL_TO_READ_RUNNING_FW_DESC;
            goto _checked;
        }

        const esp_partition_t* last_invalid_partition = esp_ota_get_last_invalid_partition();
        esp_app_desc_t last_invalid_app;

        if(last_invalid_partition != NULL && esp_ota_get_partition_description(last_invalid_partition, &last_invalid_app) != ESP_OK) {
            ota_hndl->error = DISCORD_OTA_ERR_FAIL_TO_READ_INVALID_FW_DESC;
            goto _checked;
        }

        DISCORD_LOGI("Firmware versions(new=%s, running=%s, last_invalid=%s)",
            new_app.version,
            running_app.version,
            last_invalid_partition == NULL ? "NULL" : last_invalid_app.version
        );

        if(last_invalid_partition != NULL && estr_eq(new_app.version, last_invalid_app.version)) {
            ota_hndl->error = DISCORD_OTA_ERR_NEW_VER_SAME_AS_INVALID_VER;
            goto _checked;
        }

        if(estr_eq(new_app.version, running_app.version)) {
            ota_hndl->error = DISCORD_OTA_ERR_NEW_VER_SAME_AS_RUNNING_VER;
            goto _checked;
        }
        
_checked:
        ota_hndl->image_was_checked = true;
        return true;
    }

    return false;
}

static esp_err_t ota_download_handler(discord_download_info_t* file, void* arg) {
    discord_ota_handle_t ota_hndl = (discord_ota_handle_t) arg;
    esp_err_t err = ESP_OK;

    if(!ota_hndl->image_was_checked && !ota_image(file, ota_hndl)) {
        goto _continue;
    }

    if(ota_hndl->error != DISCORD_OTA_OK) { // error was happen. discarding chunks...
        goto _error;
    }

    if(!ota_hndl->update_handle) {
        DISCORD_LOGI("Firmware downloading (size=%d B)...", file->total_length);

        if(esp_ota_begin(ota_hndl->update_partition, OTA_WITH_SEQUENTIAL_WRITES, &ota_hndl->update_handle) != ESP_OK) {
            ota_hndl->error = DISCORD_OTA_ERR_FAIL_TO_BEGIN;
            goto _error;
        }
    }

    if(ota_hndl->buffer) { // first chunk is in buffer
        if(esp_ota_write(ota_hndl->update_handle, ota_hndl->buffer, ota_hndl->buffer_offset) != ESP_OK) {
            ota_hndl->error = DISCORD_OTA_ERR_FAIL_TO_WRITE;
            goto _error; 
        } else {
            // free buffer, no longer needed
            free(ota_hndl->buffer);
            ota_hndl->buffer = NULL;
            ota_hndl->buffer_offset = 0;
            goto _continue;
        }
    }

    const int down_size = file->offset + file->length;
    ota_hndl->download_percentage = down_size * 100 / file->total_length;

    if(ota_hndl->download_percentage - ota_hndl->download_percentage_checkpoint > 8
        || down_size == file->total_length) {
        DISCORD_LOGI("Downloaded %d%%", ota_hndl->download_percentage);
        ota_hndl->download_percentage_checkpoint = ota_hndl->download_percentage;
    }

    // other chunks will be streamed directly to update partition
    if(esp_ota_write(ota_hndl->update_handle, file->data, file->length) != ESP_OK) {
        ota_hndl->error = DISCORD_OTA_ERR_FAIL_TO_WRITE;
        goto _error;
    }
    
    goto _continue;
_error:
    if(err == ESP_OK) { err = ESP_FAIL; }
    if(ota_hndl->update_handle) { 
        esp_ota_abort(ota_hndl->update_handle);
        ota_hndl->update_handle = 0;
    }
_continue:
    return err;
}

static char* partition_sha256(const uint8_t* hash, int hash_len) {
    char* sha256 = malloc(hash_len * 2 + 1);

    for (int i = 0; i < hash_len; i++) {
        sprintf(sha256 + (i * 2), "%02x", hash[i]);
    }

    sha256[hash_len * 2] = '\0';
    return sha256;
}

static esp_err_t ota_status_message_content(discord_ota_handle_t ota_hndl, char** out_content) {
    if(!ota_hndl || !out_content) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = ESP_OK;

    const esp_partition_t *running_partition = esp_ota_get_running_partition();
    esp_app_desc_t rapp;
    if ((err = esp_ota_get_partition_description(running_partition, &rapp)) != ESP_OK) {
        return err;
    }
    
    char* rapp_sha = partition_sha256(rapp.app_elf_sha256, sizeof(rapp.app_elf_sha256));

    char uptime[25];
    sprintf(uptime, "%lld ms", esp_timer_get_time() / 1000);

    char free_heap[15];
    sprintf(free_heap, "%d bytes", esp_get_free_heap_size());

    char* content = estr_cat(
        "```yaml"
        , "\nUptime: ", uptime
        , "\nFreeHeap: ", free_heap
        , "\nRunningApp:"
        , "\n  Version: ", rapp.version
        , "\n  CompileDateTime: ", rapp.date, " ", rapp.time
        , "\n  IdfVersion: ", rapp.idf_ver
        , "\n  Sha256: ", rapp_sha
        , "\n```"
    );

    free(rapp_sha);

    *out_content = content;
    return err;
}

/**
 * @brief Performs Discord OTA update
 * @param client Discord bot handle
 * @param firmware_message Message that contains new firmware as a attachment
 * @param config Configuration for OTA update. Provide NULL for default configurations
 * @return ESP_OK on success
 */
static esp_err_t discord_ota_perform(discord_handle_t client, discord_message_t* firmware_message) {
    if(!client || !client->ota || !firmware_message) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = ESP_OK;
    bool quiet = false;
    discord_ota_handle_t ota = client->ota;
    char** cmd_pieces = NULL;
    size_t cmd_pieces_len = 0;
    char* subcmd = NULL;

    if(firmware_message->author->bot) { // ignore messages from other bots
        goto _return;
    }

    if(ota->config->prefix == NULL) {
        ota->config->prefix = strdup(DISCORD_OTA_DEFAULT_PREFIX);
    }

    const int prefix_len = strlen(ota->config->prefix);

    if(prefix_len < 4) {
        ota->error = DISCORD_OTA_ERR_INVALID_OTA_MSG_PREFIX_LENGHT;
        goto _error_quiet;
    }

    if(!estr_sw(firmware_message->content, ota->config->prefix)) { // message does not starts with prefix
        goto _return; // ignore message
    }

    DISCORD_LOGI("Triggered");

    cmd_pieces = estr_split(firmware_message->content, ' ', &cmd_pieces_len);

    if(cmd_pieces_len != 3) {
        ota->error = DISCORD_OTA_ERR_INVALID_COMMAND_FORMAT;
        goto _error;
    }

    if(!estr_eq(cmd_pieces[1], "EVERYONE")) { // not for everyone
        discord_message_word_t* tagged_usr_wrd = NULL;
        discord_message_word_parse(cmd_pieces[1], &tagged_usr_wrd);

        if(tagged_usr_wrd->type != DISCORD_MESSAGE_WORD_USER
            && tagged_usr_wrd->type != DISCORD_MESSAGE_WORD_USER_NICKNAME) {
            ota->error = DISCORD_OTA_ERR_INVALID_COMMAND_FORMAT;
            goto _error;
        }

        const discord_session_t* session = NULL;
        discord_session_get_current(client, &session);

        if(!estrn_eq(session->user->id, tagged_usr_wrd->id, tagged_usr_wrd->id_len)) { // not for us
            goto _return; // ignore message
        }
    }

    subcmd = strdup(cmd_pieces[2]);
    cu_list_free(cmd_pieces, cmd_pieces_len);

    if(ota->config->channel) {
        if(ota->config->channel->id) { // Channel Id has higher priority over Name
            if(!estr_eq(ota->config->channel->id, firmware_message->channel_id)) {
                ota->error = DISCORD_OTA_ERR_OTA_WRONG_CHANNEL;
                goto _error;
            } else {
                goto _channel_ok;
            }
        }

        if(!ota->config->channel->name) {
            // there is no channel Id nor Name
            ota->error = DISCORD_OTA_ERR_INVALID_CHANNEL_CONFIG;
            goto _error_quiet;
        }

        discord_channel_t** channels = NULL;
        int channels_len = 0;
        if((err = discord_guild_get_channels(
            client,
            &(discord_guild_t) { .id = firmware_message->guild_id },
            &channels,
            &channels_len
        )) != ESP_OK) {
            ota->error = DISCORD_OTA_ERR_FAIL_TO_FETCH_CHANNELS;
            goto _error_quiet;
        }

        discord_channel_t* channel = discord_channel_get_from_array_by_name(
            channels, channels_len, ota->config->channel->name
        );

        bool channel_found = channel != NULL;
        bool correct_channel = channel_found && estr_eq(channel->id, firmware_message->channel_id);

        cu_list_freex(channels, channels_len, discord_channel_free);

        if(!correct_channel) {
            ota->error = DISCORD_OTA_ERR_OTA_WRONG_CHANNEL;
            goto _error;
        }
    }

_channel_ok:

    if(!ota->config->administrator_only_disabled) {
        bool is_admin;

        DISCORD_LOGD("Checking admin permissions...");

        if((err = discord_member_has_permissions(
            client,
            firmware_message->member,
            firmware_message->guild_id,
            DISCORD_PERMISSION_ADMINISTRATOR,
            &is_admin
        )) != ESP_OK) {
            ota->error = DISCORD_OTA_ERR_FAIL_TO_CHECK_ADMIN_PERMISSIONS;
            goto _error;
        }

        if(!is_admin) {
            ota->error = DISCORD_OTA_ERR_ADMIN_PERMISSIONS_REQUIRED;
            goto _error;
        }
    }

    if(estr_sw(subcmd, "update")) {
        goto _ota_update;
    }

    if(estr_sw(subcmd, "status")) {
        char* msg_content = NULL;

        if(ota_status_message_content(ota, &msg_content) != ESP_OK) {
            ota->error = DISCORD_OTA_ERR_FAIL_TO_CONSTRUCT_OTA_STATUS;
            goto _error;
        }

        discord_message_send(client, &(discord_message_t){
            .channel_id = firmware_message->channel_id,
            .content = msg_content
        }, NULL);

        free(msg_content);
    } else {
        ota->error = DISCORD_OTA_ERR_UNKNOWN_SUBCOMMAND;
        goto _error;
    }

    goto _return;
    
_ota_update:

    if(firmware_message->_attachments_len != 1) {
        ota->error = DISCORD_OTA_ERR_INVALID_NUM_OF_MSG_ATTACHMENTS;
        err = ESP_ERR_INVALID_ARG;
        goto _error;
    }

    // Take first attachment as a new firmware
    discord_attachment_t* firmware = firmware_message->attachments[0];

    if(!estr_ew(firmware->filename, ".bin")) {
        ota->error = DISCORD_OTA_ERR_INVALID_FW_FILE_TYPE;
        err = ESP_ERR_INVALID_ARG;
        goto _error;
    }

    ota->update_partition = esp_ota_get_next_update_partition(NULL);

    if(ota->update_partition == NULL) {
        ota->error = DISCORD_OTA_ERR_FAIL_TO_FIND_UPDATE_PARTITION;
        goto _error;
    }

    if(firmware->size > ota->update_partition->size) {
        ota->error = DISCORD_OTA_ERR_NEW_FW_CANNOT_FIT_INTO_PARTITION;
        goto _error;
    }

    // allocate new buffer
    if(!(ota->buffer = malloc(DISCORD_OTA_BUFFER_SIZE))) {
        err = ESP_ERR_NO_MEM;
        goto _error;
    }

    DISCORD_LOGI("Gathering new firmware informations...");

    if((err = discord_message_download_attachment(client, firmware_message, 0, &ota_download_handler, ota)) != ESP_OK) {
        ota->error = DISCORD_OTA_ERR_FAIL_TO_DOWNLOAD_FW;
        goto _error;
    }

    if(ota->error != DISCORD_OTA_OK) {
        DISCORD_LOGW("OTA terminated");
        goto _error;
    }

    DISCORD_LOGI("Validating...");

    if((err = esp_ota_end(ota->update_handle)) != ESP_OK) {
        if(err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ota->error = DISCORD_OTA_ERR_IMAGE_CORRUPTED;
        } else {
            ota->error = DISCORD_OTA_ERR_FAIL_TO_END;
        }

        goto _error;
    }

    DISCORD_LOGI("Mounting...");

    if((err = esp_ota_set_boot_partition(ota->update_partition)) != ESP_OK) {
        ota->error = DISCORD_OTA_ERR_FAIL_TO_MOUNT;
        goto _error;
    }

    const char* success_msg = "New firmware mounted. Rebooting...";
    DISCORD_LOGI("%s", success_msg);

    if(!ota->config->success_feedback_disabled) {
        char* success_content = estr_cat(DISCORD_EMOJI_WHITE_CHECK_MARK " OTA success: `", success_msg, "`");
        discord_message_send(client, &(discord_message_t){
            .channel_id = firmware_message->channel_id,
            .content = success_content
        }, NULL);
        free(success_content);
    }

    esp_restart();

    err = ESP_OK;
    goto _return;
_error_quiet:
    quiet = true; // be quiet
_error:
    if(err == ESP_OK) { err = ESP_FAIL; }
    if(ota->update_handle) { 
        esp_ota_abort(ota->update_handle);
        ota->update_handle = 0;
    }

    if(ota->error == DISCORD_OTA_OK) {
        ota->error = DISCORD_OTA_FAIL;
    }

    const char* error_msg = discord_ota_err_string[ota->error];
    DISCORD_LOGW("%s", error_msg);

    if(!quiet && !ota->config->error_feedback_disabled) {
        char* err_content = estr_cat(DISCORD_EMOJI_X " OTA error: `", error_msg, "`");
        discord_message_send(client, &(discord_message_t){
            .channel_id = firmware_message->channel_id,
            .content = err_content
        }, NULL);
        free(err_content);
    }
_return:
    cu_list_free(cmd_pieces, cmd_pieces_len); // no nullcheck needed
    free(subcmd);
    ota_state_reset(client);
    DISCORD_LOGI("Finished");
    return err;
}

esp_err_t discord_ota_keep(bool keep_or_rollback) {
    esp_err_t err = ESP_OK;
    esp_ota_img_states_t ota_state;
    const esp_partition_t* running_partition = esp_ota_get_running_partition();

    if ((err = esp_ota_get_state_partition(running_partition, &ota_state)) != ESP_OK) {
        return err;
    }

    if (ota_state != ESP_OTA_IMG_PENDING_VERIFY) {
        return err;
    }

    DISCORD_LOGW("New firmware version is going to be %s", 
        keep_or_rollback ? "preserved" : "rolled back"
    );

    if (keep_or_rollback) {
        esp_ota_mark_app_valid_cancel_rollback();
    } else {
        esp_ota_mark_app_invalid_rollback_and_reboot();
    }

    return err;
}

void discord_ota_config_free(discord_ota_handle_t ota) {
    if(!ota || !ota->config) {
        return;
    }

    free(ota->config->prefix);
    discord_channel_free(ota->config->channel);
    free(ota->config);
    ota->config = NULL;
}

void discord_ota_destroy(discord_handle_t client) {
    if(!client || !client->ota) {
        return;
    }

    discord_ota_handle_t ota = client->ota;
    if(ota->update_handle) { 
        esp_ota_abort(ota->update_handle);
    }
    discord_ota_config_free(ota);
    free(ota->buffer);
    discord_unregister_events(client, DISCORD_EVENT_CONNECTED, ota_on_connected);
    discord_unregister_events(client, DISCORD_EVENT_MESSAGE_RECEIVED, ota_on_message);
    discord_unregister_events(client, DISCORD_EVENT_DISCONNECTED, ota_on_disconnected);
    free(ota);
    client->ota = NULL;
}

static bool token_loaded_from_nvs = false;

/**
 * @brief Read token from NVS or from Configuration. Make sure that you have initilized
 *        NVS with nvs_flash_init() before calling this function. Token value will be
 *        dinamically alocated and needs to be released manually with free() function
 * @return Return token from NVS. If not exist in NVS, one from configuration will be returned
 */
static discord_ota_token_t discord_ota_get_token() {
    discord_ota_token_t token = {
        .err = ESP_OK  
    };

    nvs_handle_t nvs;
    char* nvs_token = NULL;
    size_t nvs_token_len = 0;

    if((token.err = nvs_open(DISCORD_NVS_NAMESPACE, NVS_READONLY, &nvs)) == ESP_OK) {
        nvs_get_str(nvs, DISCORD_NVS_KEY_TOKEN, NULL, &nvs_token_len);

        if(nvs_token_len > 0) {
            nvs_token = malloc(nvs_token_len);
            nvs_get_str(nvs, DISCORD_NVS_KEY_TOKEN, nvs_token, &nvs_token_len);
        }

        nvs_close(nvs);
    }

    if(token.err == ESP_ERR_NVS_NOT_FOUND) {
        token.err = ESP_OK;
    }

    if(nvs_token) {
        token.type = DISCORD_OTA_TOKEN_FROM_NVS;
        token.val = nvs_token;
        token_loaded_from_nvs = true;
    } else {
        token.type = DISCORD_OTA_TOKEN_FROM_CONFIG;
        token.val = strdup(CONFIG_DISCORD_TOKEN);
    }

    DISCORD_LOGI("Token loaded from %s",
        token.type == DISCORD_OTA_TOKEN_FROM_NVS ? "NVS" : "Configuration"
    );

    return token;
}

/**
 * @brief This function should be called in DISCORD_EVENT_CONNECTED event.
 *        Function will copy token from Configuration to NVS.
 *        If token is already in NVS, function will do nothing.
 * @param client Discord bot handle
 * @return ESP_OK on success
 */
static esp_err_t discord_ota_connected_handler(discord_handle_t client) {
    if(!client) {
        return ESP_ERR_INVALID_ARG;
    }

    if(token_loaded_from_nvs) {
        return ESP_OK;
    }

    esp_err_t err = ESP_OK;
    nvs_handle_t nvs;
    if((err = nvs_open(DISCORD_NVS_NAMESPACE, NVS_READWRITE, &nvs)) != ESP_OK) {
        return err;
    }
    nvs_set_str(nvs, DISCORD_NVS_KEY_TOKEN, CONFIG_DISCORD_TOKEN);
    nvs_commit(nvs);
    nvs_close(nvs);

    return err;
}

/**
 * @brief This function should be called in DISCORD_EVENT_DISCONNECTED event.
 *        If token is invalid, function will remove them from NVS.
 *        If token is still valid, function will do nothing.
 * @param client Discord bot handle
 * @return ESP_OK on success
 */
static esp_err_t discord_ota_disconnected_handler(discord_handle_t client) {
    if(!client) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = ESP_OK;
    discord_close_code_t close_code;
    discord_get_close_code(client, &close_code);
    if(close_code != DISCORD_CLOSEOP_AUTHENTICATION_FAILED) {
        return ESP_OK;
    }

    if(!token_loaded_from_nvs) {
        return ESP_OK;
    }

    nvs_handle_t nvs;
    if((err = nvs_open(DISCORD_NVS_NAMESPACE, NVS_READWRITE, &nvs)) != ESP_OK) {
        return err;
    }
    nvs_erase_key(nvs, DISCORD_NVS_KEY_TOKEN);
    nvs_commit(nvs);
    nvs_close(nvs);

    return err;
}