#ifndef _DISCORD_ROLE_H_
#define _DISCORD_ROLE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "discord.h"

typedef uint8_t discord_role_len_t;

typedef struct {
    char* id;
    char* name;
    discord_role_len_t position;
    char* permissions;
} discord_role_t;

discord_role_t** discord_role_get_all(discord_handle_t client, const char* guild_id, discord_role_len_t* out_length);
void discord_role_free(discord_role_t* role);
bool discord_role_is_in_id_list(discord_role_t* role, char** role_ids, discord_role_len_t role_ids_len);
void discord_role_sort_list(discord_role_t** roles, discord_role_len_t len);

#ifdef __cplusplus
}
#endif

#endif