#include "esp_ota_ops.h"
#include "discord/private/_discord.h"
#include "discord_ota.h"

DISCORD_LOG_DEFINE_BASE();

// Buffer required for image header
// Once when image is checked buffer can be freed

#define DISCORD_OTA_BUFFER_SIZE 1024

typedef struct discord_ota {
    void* buffer;
    int buffer_offset;
    esp_ota_handle_t update_handle;
    const esp_partition_t* update_partition;
    bool image_was_checked;
    bool image_is_valid;
    bool error;
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
        ota_hndl->error = true;
        DISCORD_LOGW("Image header cannot fit into buffer");
        return false;
    }

    // Buffering...
    memcpy(ota_hndl->buffer + ota_hndl->buffer_offset, file->data, file->length);
    ota_hndl->buffer_offset += file->length;

    if(ota_hndl->buffer_offset > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) { // Image header is in buffer
        esp_app_desc_t new_app;
        memcpy(&new_app, ota_hndl->buffer + sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t), sizeof(esp_app_desc_t));
        
        //const esp_partition_t *configured = esp_ota_get_boot_partition();
        const esp_partition_t *running_partition = esp_ota_get_running_partition();

        esp_app_desc_t running_app;
        if (esp_ota_get_partition_description(running_partition, &running_app) != ESP_OK) {
            DISCORD_LOGE("Fail to get running fw desc");
            goto _invalid_img;
        }

        const esp_partition_t* last_invalid_partition = esp_ota_get_last_invalid_partition();
        esp_app_desc_t last_invalid_app;

        if(last_invalid_partition != NULL && esp_ota_get_partition_description(last_invalid_partition, &last_invalid_app) != ESP_OK) {
            DISCORD_LOGE("Fail to get last invalid fw desc");
            goto _invalid_img;
        }

        DISCORD_LOGI("Firmware versions(new=%s, running=%s, last_invalid=%s)",
            new_app.version,
            running_app.version,
            last_invalid_partition == NULL ? "NULL" : last_invalid_app.version
        );

        if(last_invalid_partition != NULL && estr_eq(new_app.version, last_invalid_app.version)) {
            DISCORD_LOGW("New version is the same as previously invalid one. Version of new firmware should be increased");
            goto _invalid_img;
        }

        if(estr_eq(new_app.version, running_app.version)) {
            DISCORD_LOGW("New version is the same as running one. Version of new firmware should be increased");
            goto _invalid_img;
        }

        ota_hndl->image_is_valid = true;
        goto _checked;
_invalid_img:
        ota_hndl->image_is_valid = false;
_checked:
        ota_hndl->image_was_checked = true;
        return true;
    }

    return false;
}

static esp_err_t download_handler(discord_download_info_t* file, void* arg) {
    discord_ota_handle_t ota_hndl = (discord_ota_handle_t) arg;
    esp_err_t err = ESP_OK;

    if(!ota_hndl->image_was_checked && !ota_image(file, ota_hndl)) {
        goto _continue;
    }

    if(ota_hndl->error) { // error was happen. discarding chunks...
        goto _error;
    }

    if(!ota_hndl->image_is_valid) { // image is checked but it's invalid
        goto _error;
    }

    if(! ota_hndl->update_handle) {
        DISCORD_LOGI("Firmware downloading...");
    }

    if(!ota_hndl->update_handle
        && esp_ota_begin(ota_hndl->update_partition, OTA_WITH_SEQUENTIAL_WRITES, &ota_hndl->update_handle) != ESP_OK) {
        DISCORD_LOGE("Fail to OTA begin");
        goto _error;
    }

    if(ota_hndl->buffer) { // first chunk is in buffer
        if(esp_ota_write(ota_hndl->update_handle, ota_hndl->buffer, ota_hndl->buffer_offset) != ESP_OK) {
            DISCORD_LOGE("Fail to OTA write");
            goto _error; 
        } else {
            // free buffer, no longer needed
            free(ota_hndl->buffer);
            ota_hndl->buffer = NULL;
            ota_hndl->buffer_offset = 0;
            goto _continue;
        }
    }

    // other chunks will be streamed directly to update partition
    if(esp_ota_write(ota_hndl->update_handle, file->data, file->length) != ESP_OK) {
        DISCORD_LOGE("Fail to OTA write");
        goto _error;
    }
    
    goto _continue;
_error:
    if(err == ESP_OK) { err = ESP_FAIL; }
    if(ota_hndl->update_handle) { 
        esp_ota_abort(ota_hndl->update_handle);
    }
    ota_hndl->error = true;
_continue:
    return err;
}

esp_err_t discord_ota(discord_handle_t handle, discord_message_t* firmware_message) {
    if(!handle || !firmware_message) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err;
    discord_ota_handle_t ota_handle = cu_tctor(discord_ota_handle_t, discord_ota_t);

    if(firmware_message->_attachments_len != 1) {
        DISCORD_LOGW("There should be (only) one attachment in message");
        err = ESP_ERR_INVALID_ARG;
        goto _error;
    }

    // Take first attachment as a new firmware
    discord_attachment_t* firmware = firmware_message->attachments[0];

    if(!estr_ew(firmware->filename, ".bin")) {
        DISCORD_LOGW("Invalid firmware file type");
        err = ESP_ERR_INVALID_ARG;
        goto _error;
    }

    ota_handle->update_partition = esp_ota_get_next_update_partition(NULL);

    if(ota_handle->update_partition == NULL) {
        DISCORD_LOGE("Fail to find update partition");
        err = ESP_FAIL;
        goto _error;
    }

    if(firmware->size > ota_handle->update_partition->size) {
        DISCORD_LOGE("New version of firmware cannot fit into update partition");
        err = ESP_FAIL;
        goto _error;
    }

    if(!(ota_handle->buffer = malloc(DISCORD_OTA_BUFFER_SIZE))) {
        err = ESP_ERR_NO_MEM;
        goto _error;
    }

    DISCORD_LOGI("Gathering new firmware informations...");

    if((err = discord_message_download_attachment(handle, firmware_message, 0, &download_handler, ota_handle)) != ESP_OK) {
        DISCORD_LOGE("Fail to download new firmware");
        goto _error;
    }

    if(ota_handle->error) {
        DISCORD_LOGW("OTA terminated");
        err = ESP_FAIL;
        goto _error;
    }

    DISCORD_LOGI("Validating...");

    if((err = esp_ota_end(ota_handle->update_handle)) != ESP_OK) {
        if(err == ESP_ERR_OTA_VALIDATE_FAILED) {
            DISCORD_LOGE("Image validation failed, image is corrupted");
        } else {
            DISCORD_LOGE("Fail to OTA end");
        }

        goto _error;
    }

    DISCORD_LOGI("Mounting...");

    if((err = esp_ota_set_boot_partition(ota_handle->update_partition)) != ESP_OK) {
        DISCORD_LOGE("Fail to set OTA boot partition");
        goto _error;
    }

    DISCORD_LOGI("New firmware has been successfully mounted. Restarting in 10 seconds...");

    vTaskDelay(10000 / portTICK_PERIOD_MS);
    esp_restart();

    err = ESP_OK;
    goto _return;
_error:
    if(ota_handle->update_handle) { 
        esp_ota_abort(ota_handle->update_handle);
    }
_return:
    discord_ota_free(ota_handle);
    return err;
}

static void discord_ota_free(discord_ota_handle_t hndl) {
    if(hndl == NULL) {
        return;
    }

    free(hndl->buffer);
    free(hndl);
}