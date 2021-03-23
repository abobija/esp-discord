#include "discord/guild.h"
#include "discord/private/_discord.h"
#include "discord/private/_api.h"
#include "estr.h"

void discord_guild_free(discord_guild_t* guild) {
    if(!guild)
        return;

    free(guild->id);
    free(guild->name);
    free(guild->permissions);
    free(guild);
}