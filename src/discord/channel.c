#include "discord/channel.h"

void discord_channel_free(discord_channel_t* channel) {
    if(!channel)
        return;

    free(channel->id);
    free(channel->name);
    free(channel);
}