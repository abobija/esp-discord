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
    free(embed);
}