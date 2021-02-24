#include "discord/role.h"
#include "discord/private/_discord.h"
#include "discord/private/_api.h"
#include "discord/utils.h"

DISCORD_LOG_DEFINE_BASE();

discord_role_t* discord_role_ctor(char* id, char* name, uint8_t position, char* permissions) {
    discord_role_t* role = calloc(1, sizeof(discord_role_t));

    role->id = id;
    role->name = name;
    role->position = position;
    role->permissions = permissions;

    return role;
}

discord_role_t** discord_role_get_all(discord_handle_t client, char* guild_id, uint8_t* roles_len) {
    if(!client || !guild_id || !roles_len) {
        DISCORD_LOGE("Invalid args");
        return NULL;
    }

    discord_role_t** roles = NULL;

    discord_api_response_t* res = dcapi_get(
        client,
        discord_strcat("/guilds/", guild_id, "/roles"),
        NULL,
        true
    );

    if(dcapi_response_is_success(res) && res->data_len > 0) {
        roles = discord_role_list_deserialize(res->data, res->data_len, roles_len);
    }

    dcapi_response_free(client, res);

    return roles;
}

void discord_role_free(discord_role_t* role) {
    if(!role)
        return;

    free(role->id);
    free(role->name);
    free(role->permissions);
    free(role);
}

void discord_role_list_free(discord_role_t** roles, uint8_t len) {
    if(!roles)
        return;
    
    for(uint8_t i = 0; i < len; i++) {
        discord_role_free(roles[i]);
    }

    free(roles);
}