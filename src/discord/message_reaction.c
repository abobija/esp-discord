#include "discord/message_reaction.h"
#include "esp_heap_caps.h"

discord_message_reaction_t* discord_message_reaction_ctor(char* user_id, char* message_id, char* channel_id, discord_emoji_t* emoji) {
    discord_message_reaction_t* react = calloc(1, sizeof(discord_message_reaction_t));

    react->user_id = user_id;
    react->message_id = message_id;
    react->channel_id = channel_id;
    react->emoji = emoji;

    return react;
}

void discord_message_reaction_free(discord_message_reaction_t* reaction) {
    if(!reaction)
        return;

    free(reaction->user_id);
    free(reaction->message_id);
    free(reaction->channel_id);
    discord_emoji_free(reaction->emoji);
    free(reaction);
}