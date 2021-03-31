#include "esp_ota_ops.h"
#include "discord/private/_discord.h"
#include "discord_ota.h"

DISCORD_LOG_DEFINE_BASE();

#define DCOTA_FOREACH_ERR(ERR)                            \
    ERR(DISCORD_OTA_OK)                                   \
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
    ERR(DISCORD_OTA_ERR_INVALID_OTA_MSG_PREFIX_LENGHT)    \

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

typedef struct discord_ota {
    discord_ota_config_t* config;
    void* buffer;
    int buffer_offset;
    esp_ota_handle_t update_handle;
    const esp_partition_t* update_partition;
    bool image_was_checked;
    discord_ota_err_t error;
    int download_percentage;
    int download_percentage_checkpoint;
} discord_ota_t;

typedef discord_ota_t* discord_ota_handle_t;

static void discord_ota_free(discord_ota_handle_t hndl);

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

esp_err_t discord_ota(discord_handle_t client, discord_message_t* firmware_message, discord_ota_config_t* config) {
    if(!client || !firmware_message) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = ESP_OK;

    discord_ota_handle_t ota_handle = cu_tctor(discord_ota_handle_t, discord_ota_t,
        .config = cu_ctor(discord_ota_config_t)
    );

    if(config) {
        ota_handle->config->prefix = STRDUP(config->prefix);
        ota_handle->config->success_feedback_disabled = config->success_feedback_disabled;
        ota_handle->config->error_feedback_disabled = config->error_feedback_disabled;
        ota_handle->config->administrator_only_disabled = config->administrator_only_disabled;
        ota_handle->config->channel = config->channel;
    }

    if(ota_handle->config->prefix == NULL) {
        ota_handle->config->prefix = strdup(DISCORD_OTA_DEFAULT_PREFIX);
    }

    if(strlen(ota_handle->config->prefix) < 3) {
        ota_handle->error = DISCORD_OTA_ERR_INVALID_OTA_MSG_PREFIX_LENGHT;
        goto _error_quiet;
    }

    if(!estr_sw(firmware_message->content, ota_handle->config->prefix)) {
        goto _return; // ignore message
    }

    if(ota_handle->config->channel) {
        if(ota_handle->config->channel->id) { // Channel Id has higher priority over Name
            if(!estr_eq(ota_handle->config->channel->id, firmware_message->channel_id)) {
                goto _return; // ignore message
            } else {
                goto _channel_ok;
            }
        }

        if(!ota_handle->config->channel->name) {
            // there is no channel Id nor Name
            ota_handle->error = DISCORD_OTA_ERR_INVALID_CHANNEL_CONFIG;
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
            ota_handle->error = DISCORD_OTA_ERR_FAIL_TO_FETCH_CHANNELS;
            goto _error_quiet;
        }

        discord_channel_t* channel = discord_channel_get_from_array_by_name(
            channels, channels_len, ota_handle->config->channel->name
        );

        bool channel_found = channel != NULL;
        bool correct_channel = channel_found && estr_eq(channel->id, firmware_message->channel_id);

        cu_list_freex(channels, channels_len, discord_channel_free);

        if(!correct_channel) {
            goto _return; // ignore message
        }
    }
_channel_ok:

    if(firmware_message->_attachments_len != 1) {
        ota_handle->error = DISCORD_OTA_ERR_INVALID_NUM_OF_MSG_ATTACHMENTS;
        err = ESP_ERR_INVALID_ARG;
        goto _error;
    }

    if(!ota_handle->config->administrator_only_disabled) { // only admin can perform update
        bool is_admin;

        DISCORD_LOGI("Checking admin permissions...");

        if((err = discord_member_has_permissions(
            client,
            firmware_message->member,
            firmware_message->guild_id,
            DISCORD_PERMISSION_ADMINISTRATOR,
            &is_admin
        )) != ESP_OK) {
            ota_handle->error = DISCORD_OTA_ERR_FAIL_TO_CHECK_ADMIN_PERMISSIONS;
            goto _error;
        }

        if(!is_admin) {
            ota_handle->error = DISCORD_OTA_ERR_ADMIN_PERMISSIONS_REQUIRED;
            goto _error;
        }
    }

    // Take first attachment as a new firmware
    discord_attachment_t* firmware = firmware_message->attachments[0];

    if(!estr_ew(firmware->filename, ".bin")) {
        ota_handle->error = DISCORD_OTA_ERR_INVALID_FW_FILE_TYPE;
        err = ESP_ERR_INVALID_ARG;
        goto _error;
    }

    ota_handle->update_partition = esp_ota_get_next_update_partition(NULL);

    if(ota_handle->update_partition == NULL) {
        ota_handle->error = DISCORD_OTA_ERR_FAIL_TO_FIND_UPDATE_PARTITION;
        goto _error;
    }

    if(firmware->size > ota_handle->update_partition->size) {
        ota_handle->error = DISCORD_OTA_ERR_NEW_FW_CANNOT_FIT_INTO_PARTITION;
        goto _error;
    }

    if(!(ota_handle->buffer = malloc(DISCORD_OTA_BUFFER_SIZE))) {
        err = ESP_ERR_NO_MEM;
        goto _error;
    }

    DISCORD_LOGI("Gathering new firmware informations...");

    if((err = discord_message_download_attachment(client, firmware_message, 0, &ota_download_handler, ota_handle)) != ESP_OK) {
        ota_handle->error = DISCORD_OTA_ERR_FAIL_TO_DOWNLOAD_FW;
        goto _error;
    }

    if(ota_handle->error != DISCORD_OTA_OK) {
        DISCORD_LOGW("OTA terminated");
        goto _error;
    }

    DISCORD_LOGI("Validating...");

    if((err = esp_ota_end(ota_handle->update_handle)) != ESP_OK) {
        if(err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ota_handle->error = DISCORD_OTA_ERR_IMAGE_CORRUPTED;
        } else {
            ota_handle->error = DISCORD_OTA_ERR_FAIL_TO_END;
        }

        goto _error;
    }

    DISCORD_LOGI("Mounting...");

    if((err = esp_ota_set_boot_partition(ota_handle->update_partition)) != ESP_OK) {
        ota_handle->error = DISCORD_OTA_ERR_FAIL_TO_MOUNT;
        goto _error;
    }

    const char* success_msg = "New firmware has been successfully mounted. Rebooting...";
    DISCORD_LOGI("%s", success_msg);

    if(!ota_handle->config->success_feedback_disabled) {
        discord_message_send(client, &(discord_message_t){
            .channel_id = firmware_message->channel_id,
            .content = (char*) success_msg
        }, NULL);
    }

    esp_restart();

    err = ESP_OK;
    goto _return;
_error_quiet:
    ota_handle->config->error_feedback_disabled = true; // be quiet
_error:
    if(err == ESP_OK) { err = ESP_FAIL; }
    if(ota_handle->update_handle) { 
        esp_ota_abort(ota_handle->update_handle);
        ota_handle->update_handle = 0;
    }

    const char* error_msg = discord_ota_err_string[ota_handle->error];
    DISCORD_LOGW("%s", error_msg);

    if(!ota_handle->config->error_feedback_disabled) {
        char* err_content = estr_cat("Error: `", error_msg, "`");
        discord_message_send(client, &(discord_message_t){
            .channel_id = firmware_message->channel_id,
            .content = err_content
        }, NULL);
        free(err_content);
    }
_return:
    discord_ota_free(ota_handle);
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

    if (keep_or_rollback) {
        esp_ota_mark_app_valid_cancel_rollback();
    } else {
        esp_ota_mark_app_invalid_rollback_and_reboot();
    }

    return err;
}

static void discord_ota_free(discord_ota_handle_t hndl) {
    if(hndl == NULL) {
        return;
    }

    free(hndl->config->prefix);
    free(hndl->config);
    free(hndl->buffer);
    if(hndl->update_handle) { 
        esp_ota_abort(hndl->update_handle);
    }
    free(hndl);
}