#include "discord/attachment.h"
#include "esp_heap_caps.h"

discord_attachment_t* discord_attachment_ctor(char* id, char* filename, char* content_type, size_t size, char* url) {
    discord_attachment_t* attachment = calloc(1, sizeof(discord_attachment_t));

    attachment->id = id;
    attachment->filename = filename;
    attachment->content_type = content_type;
    attachment->size = size;
    attachment->url = url;

    return attachment;
}

void discord_attachment_free(discord_attachment_t* attachment) {
    if(!attachment)
        return;
    
    free(attachment->id);
    free(attachment->filename);
    free(attachment->content_type);
    free(attachment->url);
    free(attachment);
}