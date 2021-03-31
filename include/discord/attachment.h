#ifndef _DISCORD_ATTACHMENT_H_
#define _DISCORD_ATTACHMENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "discord.h"

typedef struct {
    char* id;
    char* filename;
    char* content_type;
    size_t size;
    char* url;
} discord_attachment_t;

#define discord_attachment_dump_log(LOG_FOO, TAG, attachment) \
    LOG_FOO(TAG, "attachment (id=%s, filename=%s, type=%s, size=%d, url=%s)", \
        attachment->id, \
        attachment->filename, \
        attachment->content_type ? attachment->content_type : "NULL", \
        attachment->size, \
        attachment->url \
    );

void discord_attachment_free(discord_attachment_t* attachment);

#ifdef __cplusplus
}
#endif

#endif