#include "discord/attachment.h"
#include "esp_heap_caps.h"

void discord_attachment_free(discord_attachment_t* attachment) {
    if(!attachment)
        return;
    
    free(attachment->id);
    free(attachment->filename);
    free(attachment->content_type);
    free(attachment->url);
    free(attachment);
}