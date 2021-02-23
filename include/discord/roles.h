#ifndef _DISCORD_ROLES_H_
#define _DISCORD_ROLES_H_

#include "discord.h"
#include "discord/models/role.h"

#ifdef __cplusplus
extern "C" {
#endif

discord_role_t** discord_role_get_all(discord_handle_t client, char* guild_id, uint8_t* roles_len);

#ifdef __cplusplus
}
#endif

#endif