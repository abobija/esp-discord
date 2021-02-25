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

void discord_attachment_free(discord_attachment_t* attachment);

#ifdef __cplusplus
}
#endif

#endif