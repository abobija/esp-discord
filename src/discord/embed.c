#include "discord/embed.h"

static void discord_embed_image_free(discord_embed_image_t* image)
{
    if(!image)
        return;
    
    free(image->url);
    free(image);
}

static void discord_embed_footer_free(discord_embed_footer_t* footer)
{
    if(!footer)
        return;
    
    free(footer->text);
    free(footer->icon_url);
    free(footer);
}

static void discord_embed_author_free(discord_embed_author_t* author)
{
    if(!author)
        return;
    
    free(author->name);
    free(author->url);
    free(author->icon_url);
    free(author);
}

static void discord_embed_field_free(discord_embed_field_t* field)
{
    if(!field)
        return;
    
    free(field->name);
    free(field->value);
    free(field);
}

esp_err_t discord_embed_add_field(discord_embed_t* embed, discord_embed_field_t* field)
{
    if(! embed || ! field) {
        return ESP_ERR_INVALID_ARG;
    }

    embed->fields = realloc(embed->fields, ++embed->_fields_len * sizeof(discord_embed_field_t*));
    embed->fields[embed->_fields_len - 1] = field;

    return ESP_OK;
}

void discord_embed_free(discord_embed_t* embed)
{
    if(!embed)
        return;

    free(embed->title);
    free(embed->description);
    free(embed->url);
    discord_embed_footer_free(embed->footer);
    discord_embed_image_free(embed->thumbnail);
    discord_embed_image_free(embed->image);
    discord_embed_author_free(embed->author);

    for(uint8_t i = 0; i < embed->_fields_len; i++) {
        discord_embed_field_free(embed->fields[i]);
    }

    embed->_fields_len = 0;

    free(embed);
}