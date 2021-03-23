#ifndef _DISCORD_USER_H_
#define _DISCORD_USER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "discord.h"
#include "discord/guild.h"

typedef struct {
    char* id;
    bool bot;
    char* username;
    char* discriminator;
} discord_user_t;

/**
 * @brief Returns a list of partial guild objects the current user is a member of
 * @param client Discord client handle
 * @param out_guilds Pointer to outside array of guilds
 * @param out_length Pointer to variable where the lenght of the guilds array will be stored
 * @return ESP_OK on success
 */
esp_err_t discord_user_get_my_guilds(discord_handle_t client, discord_guild_t*** out_guilds, int* out_length);
void discord_user_free(discord_user_t* user);

#ifdef __cplusplus
}
#endif

#endif