#ifndef _DISCORD_EMBED_H_
#define _DISCORD_EMBED_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "discord.h"

typedef struct {
    char* title;
    char* description;
    char* url;
} discord_embed_t;

void discord_embed_free(discord_embed_t* embed);

#ifdef __cplusplus
}
#endif

#endif