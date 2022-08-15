#include "discord/attachment.h"
#include "esp_heap_caps.h"
#include "cutils.h"
#include "estr.h"
#include "string.h"

char* discord_attachment_refence(discord_attachment_t* attachment)
{
    if(!attachment || !attachment->filename) {
        return NULL;
    }

    return estr_cat("attachment://", attachment->filename);
}

/**
 * @brief Function for releasing memory occupied by attachment. Property _data will not be freed.
 * 
 * @param attachment Attachment that needs to be freed
 */
void discord_attachment_free(discord_attachment_t* attachment)
{
    if(!attachment)
        return;
    
    free(attachment->id);
    free(attachment->filename);
    free(attachment->content_type);
    free(attachment->url);

    if(attachment->_data_should_be_freed) {
        free(attachment->_data);
        attachment->size = 0;
    }

    free(attachment);
}