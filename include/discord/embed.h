#ifndef _DISCORD_EMBED_H_
#define _DISCORD_EMBED_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "discord.h"

typedef struct {
    char* text;                       /*!< Footer text. Limit: 2048 */
    char* icon_url;                   /*!< Url of footer icon (only supports http(s) and attachments) */
} discord_embed_footer_t;

typedef struct {
    char* url;                        /*!< Source url (only supports http(s) and attachments) */
} discord_embed_image_t;

typedef struct {
    char* name;                       /*!< Name of author. Limit: 256 */
    char* url;                        /*!< Url of author */
    char* icon_url;                   /*!< Url of author icon (only supports http(s) and attachments) */
} discord_embed_author_t;

typedef struct {
    char* name;                       /*!< Name of the field. Limit: 256 */
    char* value;                      /*!< Value of the field. Limit: 1024 */
    bool is_inline;                   /*!< Whether or not this field should display inline */
} discord_embed_field_t;

typedef struct {
    char* title;                      /*!< Title of embed. Limit: 256 */
    char* description;                /*!< Description of embed. Limit: 4096 */
    char* url;                        /*!< Url of embed */
    int color;                        /*!< Color code of the embed. DISCORD_COLOR_* predefines can be used */
    discord_embed_footer_t* footer;   /*!< Footer information */
    discord_embed_image_t* thumbnail; /*!< Thumbnail information */
    discord_embed_image_t* image;     /*!< Image information */
    discord_embed_author_t* author;   /*!< Author information */
    discord_embed_field_t** fields;   /*!< Fields information. Maximum fields: 25 */
    uint8_t _fields_len;
} discord_embed_t;

esp_err_t discord_embed_add_field(discord_embed_t* embed, discord_embed_field_t* field);
void discord_embed_free(discord_embed_t* embed);

#ifdef __cplusplus
}
#endif

#endif