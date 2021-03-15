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

esp_err_t discord_member_get(discord_handle_t client, char* guild_id, char* user_id, discord_member_t** out_member);
esp_err_t discord_member_has_permissions(discord_handle_t client, discord_member_t* member, char* guild_id, uint64_t permissions, bool* out_result);
esp_err_t discord_member_has_role_name(discord_handle_t client, discord_member_t* member, const char* guild_id, const char* role_name, bool* out_result);
void discord_member_free(discord_member_t* member);

#ifdef __cplusplus
}
#endif

#endif