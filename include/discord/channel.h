#ifndef _DISCORD_CHANNEL_H_
#define _DISCORD_CHANNEL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "discord.h"

typedef enum {
    DISCORD_CHANNEL_GUILD_TEXT,      /*<! a text channel within a server */
    DISCORD_CHANNEL_DM,              /*<! a direct message between users */
    DISCORD_CHANNEL_GUILD_VOICE,     /*<! a voice channel within a server */
    DISCORD_CHANNEL_GROUP_DM,        /*<! a direct message between multiple users */
    DISCORD_CHANNEL_GUILD_CATEGORY,  /*<! an organizational category that contains up to 50 channels */
    DISCORD_CHANNEL_GUILD_NEWS,      /*<! a channel that users can follow and crosspost into their own server */
    DISCORD_CHANNEL_GUILD_STORE      /*<! a channel in which game developers can sell their game on Discord */
} discord_channel_type_t;

typedef struct {
    char* id;
    discord_channel_type_t type;
    char* name;
} discord_channel_t;

discord_channel_t* discord_channel_get_from_array_by_name(discord_channel_t** array, int array_len, const char* channel_name);
void discord_channel_free(discord_channel_t* channel);

#ifdef __cplusplus
}
#endif

#endif