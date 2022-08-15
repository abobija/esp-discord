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
    char* _data;
    bool _data_should_be_freed; /*<! Set to true if _data should be freed by discord_attachment_free function */
} discord_attachment_t;

#define discord_attachment_dump_log(LOG_FOO, TAG, attachment) \
    LOG_FOO(TAG, "attachment (id=%s, filename=%s, type=%s, size=%d, url=%s)", \
        attachment->id, \
        attachment->filename, \
        attachment->content_type ? attachment->content_type : "NULL", \
        attachment->size, \
        attachment->url \
    );

/**
 * @brief Create reference string for an attachment that can be used in embeds
 * 
 * @param attachment Attachment for which reference needs to be created
 * @return Reference string that can be used in embeds
 */
char* discord_attachment_refence(discord_attachment_t* attachment);
void discord_attachment_free(discord_attachment_t* attachment);

#ifdef __cplusplus
}
#endif

#endif