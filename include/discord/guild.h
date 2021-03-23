#ifndef _DISCORD_GUILD_H_
#define _DISCORD_GUILD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "discord.h"
#include "discord/channel.h"

typedef struct {
    char* id;
    char* name;
    char* permissions;
} discord_guild_t;

/**
 * @brief Returns a list of guild channel objects
 * @param client Discord client handle
 * @param guild Guild
 * @param out_channels Pointer to outside array of channels
 * @param out_length Pointer to variable where the lenght of the channels array will be stored
 * @return ESP_OK on success
 */
esp_err_t discord_guild_get_channels(discord_handle_t client, discord_guild_t* guild, discord_channel_t*** out_channels, int* out_length);
void discord_guild_free(discord_guild_t* guild);

#ifdef __cplusplus
}
#endif

#endif