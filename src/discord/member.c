#include "discord/member.h"
#include "discord/private/_discord.h"
#include "discord/private/_api.h"
#include "discord/private/_json.h"
#include "discord/utils.h"

DISCORD_LOG_DEFINE_BASE();

discord_member_t* discord_member_get(discord_handle_t client, char* guild_id, char* user_id) {
    if(!client || !guild_id || !user_id) {
        DISCORD_LOGE("Invalid args");
        return NULL;
    }

    discord_member_t* member = NULL;

    discord_api_response_t* res = dcapi_get(
        client,
        discord_strcat("/guilds/", guild_id, "/members/", user_id),
        NULL,
        true
    );

    if(dcapi_response_is_success(res) && res->data_len > 0) {
        member = discord_json_deserialize_(member, res->data, res->data_len);
    }

    dcapi_response_free(client, res);

    return member;
}

void discord_member_free(discord_member_t* member) {
    if(!member)
        return;

    free(member->nick);
    free(member->permissions);
    discord_list_free(member->roles, member->_roles_len, free);
    free(member);
}

bool discord_member_has_permissions_(discord_handle_t client, discord_member_t* member, discord_role_t** roles, discord_role_len_t roles_len, uint64_t permissions, esp_err_t* err) {
    if(!client || !member || !roles || roles_len <= 0) {
        DISCORD_LOGE("Invalid args");
        if(err) *err = ESP_ERR_INVALID_ARG;
        return false;
    }

    if(err) *err = ESP_OK;

    discord_role_sort_list(roles, roles_len);

    uint64_t o_ring = strtoull(roles[0]->permissions, NULL, 10); // @everyone permissions

    if((o_ring & DISCORD_PERMISSION_ADMINISTRATOR) == DISCORD_PERMISSION_ADMINISTRATOR) { // @everyone has administrator
        return true;
    }

    if(member->_roles_len > 0) {
        for(discord_role_len_t i = 1; i < roles_len; i++) {
            if(! discord_role_is_in_id_list(roles[i], member->roles, member->_roles_len)) { // member does not have this role
                continue;
            }

            o_ring |= strtoull(roles[i]->permissions, NULL, 10);

            if((o_ring & DISCORD_PERMISSION_ADMINISTRATOR) == DISCORD_PERMISSION_ADMINISTRATOR) {
                return true;
            }
        }
    }
    
    return (o_ring & permissions) == permissions;
}

bool discord_member_has_permissions(discord_handle_t client, discord_member_t* member, char* guild_id, uint64_t permissions, esp_err_t* err) {
    if(!client || !member || !guild_id) {
        DISCORD_LOGE("Invalid args");
        if(err) *err = ESP_ERR_INVALID_ARG;
        return false;
    }

    discord_role_len_t len;
    discord_role_t** roles = discord_role_get_all(client, guild_id, &len);

    if(!roles) {
        DISCORD_LOGW("Fail to fetch");
        if(err) *err = ESP_FAIL;
        return false;
    }
    
    bool res = discord_member_has_permissions_(client, member, roles, len, permissions, err);
    discord_role_list_free(roles, len);

    return res;
}