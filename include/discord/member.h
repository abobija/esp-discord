#ifndef _DISCORD_MEMBER_H_
#define _DISCORD_MEMBER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "discord.h"

typedef struct {
    char* nick;
    char* permissions;
    char** roles;
    uint8_t _roles_len;
} discord_member_t;

discord_member_t* discord_member_get(discord_handle_t client, char* guild_id, char* user_id);
void discord_member_free(discord_member_t* member);

#ifdef __cplusplus
}
#endif

#endif