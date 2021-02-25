#include "discord/role.h"
#include "discord/private/_discord.h"
#include "discord/private/_api.h"
#include "discord/private/_json.h"
#include "discord/utils.h"

DISCORD_LOG_DEFINE_BASE();

discord_role_t** discord_role_get_all(discord_handle_t client, const char* guild_id, discord_role_len_t* out_length) {
    if(!client || !guild_id || !out_length) {
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
        roles = discord_json_list_deserialize(role, res->data, res->data_len, out_length);
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

void discord_role_list_free(discord_role_t** roles, discord_role_len_t len) {
    if(!roles)
        return;
    
    for(discord_role_len_t i = 0; i < len; i++) {
        discord_role_free(roles[i]);
    }

    free(roles);
}

bool discord_role_is_in_id_list(discord_role_t* role, char** role_ids, discord_role_len_t role_ids_len) {
    for(discord_role_len_t i = 0; i < role_ids_len; i++) {
        if(discord_streq(role_ids[i], role->id)) {
            return true;
        }
    }

    return false;
}

static int dc_role_cmp(const void* role1, const void* role2) {
    return (*(discord_role_t**) role1)->position - (*(discord_role_t**) role2)->position;
}

void discord_role_sort_list(discord_role_t** roles, discord_role_len_t len) {
    qsort(roles, len, sizeof(discord_role_t*), dc_role_cmp);
}