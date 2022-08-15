#ifndef _DISCORD_EMBED_H_
#define _DISCORD_EMBED_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "discord.h"

typedef struct {
    char* text;      /*!< Footer text */
    char* icon_url;  /*!< Url of footer icon (only supports http(s) and attachments) */
} discord_embed_footer_t;

typedef struct {
    char* title;
    char* description;
    char* url;
    int color;
    discord_embed_footer_t* footer;
} discord_embed_t;

void discord_embed_free(discord_embed_t* embed);

#ifdef __cplusplus
}
#endif

#endif