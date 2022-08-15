#include "discord/embed.h"

void discord_embed_free(discord_embed_t* embed) {
    if(!embed)
        return;

    free(embed->title);
    free(embed->description);
    free(embed);
}