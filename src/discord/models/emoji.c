#include "discord/models/emoji.h"
#include "esp_heap_caps.h"

void discord_model_emoji_free(discord_emoji_t* emoji) {
    if(!emoji)
        return;

    free(emoji->name);
    free(emoji);
}