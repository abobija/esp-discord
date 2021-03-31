#include "discord/channel.h"
#include "estr.h"

discord_channel_t* discord_channel_get_from_array_by_name(discord_channel_t** array, int array_len, const char* channel_name) {
    if(!array || array_len <= 0 || !channel_name) {
        return NULL;
    }

    for(int i = 0; i < array_len; i++) {
        if(estr_eq(array[i]->name, channel_name)) {
            return array[i];
        }
    }

    return NULL;
}

void discord_channel_free(discord_channel_t* channel) {
    if(!channel)
        return;

    free(channel->id);
    free(channel->name);
    free(channel);
}