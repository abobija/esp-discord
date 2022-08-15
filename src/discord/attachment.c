#include "discord/attachment.h"
#include "esp_heap_caps.h"
#include "cutils.h"
#include "string.h"

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
    free(attachment);
}