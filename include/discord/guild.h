#ifndef _DISCORD_GUILD_H_
#define _DISCORD_GUILD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "discord.h"

typedef struct {
    char* id;
    char* name;
    char* permissions;
} discord_guild_t;

void discord_guild_free(discord_guild_t* guild);

#ifdef __cplusplus
}
#endif

#endif