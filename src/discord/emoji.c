#include "discord/emoji.h"
#include "esp_heap_caps.h"

discord_emoji_t* discord_emoji_ctor(char* name) {
    discord_emoji_t* emoji = calloc(1, sizeof(discord_emoji_t));

    emoji->name = name;

    return emoji;
}

void discord_emoji_free(discord_emoji_t* emoji) {
    if(!emoji)
        return;

    free(emoji->name);
    free(emoji);
}