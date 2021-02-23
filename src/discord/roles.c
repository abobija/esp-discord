#include "discord/roles.h"
#include "discord/private/_discord.h"
#include "discord/private/_api.h"
#include "discord/utils.h"

DISCORD_LOG_DEFINE_BASE();

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
        roles = discord_role_deserialize_list(res->data, res->data_len, roles_len);
    }

    dcapi_response_free(client, res);

    return roles;
}