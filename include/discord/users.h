#ifndef _DISCORD_USERS_H_
#define _DISCORD_USERS_H_

#include "discord.h"
#include "discord/models/member.h"

#ifdef __cplusplus
extern "C" {
#endif

discord_member_t* discord_member_get(discord_handle_t client, char* guild_id, char* user_id);

#ifdef __cplusplus
}
#endif

#endif