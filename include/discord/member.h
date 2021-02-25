#ifndef _DISCORD_MEMBER_H_
#define _DISCORD_MEMBER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "discord.h"
#include "discord/role.h"

typedef struct {
    char* nick;
    char* permissions;
    char** roles;
    discord_role_len_t _roles_len;
} discord_member_t;

discord_member_t* discord_member_get(discord_handle_t client, char* guild_id, char* user_id);
void discord_member_free(discord_member_t* member);
bool discord_member_has_permissions_(discord_handle_t client, discord_member_t* member, discord_role_t** roles, discord_role_len_t roles_len, uint64_t permissions, esp_err_t* err);
bool discord_member_has_permissions(discord_handle_t client, discord_member_t* member, char* guild_id, uint64_t permissions, esp_err_t* err);

#ifdef __cplusplus
}
#endif

#endif