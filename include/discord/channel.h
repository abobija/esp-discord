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
    GUILD_NEWS_THREAD = 10,          /*<! a temporary sub-channel within a GUILD_NEWS channel */
    GUILD_PUBLIC_THREAD,             /*<! a temporary sub-channel within a GUILD_TEXT channel */
    GUILD_PRIVATE_THREAD,            /*<! a temporary sub-channel within a GUILD_TEXT channel that is only viewable by those invited and those with the MANAGE_THREADS permission */
    GUILD_STAGE_VOICE,               /*<! a voice channel for hosting events with an audience */
    GUILD_DIRECTORY,                 /*<! the channel in a hub containing the listed servers */
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