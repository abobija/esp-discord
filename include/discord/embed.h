#ifndef _DISCORD_EMBED_H_
#define _DISCORD_EMBED_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "discord.h"

typedef struct {
    char* text;                       /*!< Footer text */
    char* icon_url;                   /*!< Url of footer icon (only supports http(s) and attachments) */
} discord_embed_footer_t;

typedef struct {
    char* url;                        /*!< Source url (only supports http(s) and attachments) */
} discord_embed_image_t;

typedef struct {
    char* title;                      /*!< Title of embed. Limit: 256 */
    char* description;                /*!< Description of embed. Limit: 4096 */
    char* url;                        /*!< Url of embed */
    int color;                        /*!< Color code of the embed. DISCORD_COLOR_* predefines can be used */
    discord_embed_footer_t* footer;   /*!< Footer information */
    discord_embed_image_t* thumbnail; /*!< Thumbnail information */
    discord_embed_image_t* image;     /*!< Image information */
} discord_embed_t;

void discord_embed_free(discord_embed_t* embed);

#ifdef __cplusplus
}
#endif

#endif