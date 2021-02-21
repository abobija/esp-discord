#include "discord/models/message_reaction.h"
#include "esp_heap_caps.h"

void discord_model_message_reaction_free(discord_message_reaction_t* reaction) {
    if(!reaction)
        return;

    free(reaction->user_id);
    free(reaction->message_id);
    free(reaction->channel_id);
    discord_model_emoji_free(reaction->emoji);
    free(reaction);
}