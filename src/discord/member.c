#include "discord/member.h"
#include "discord/private/_discord.h"
#include "discord/private/_api.h"
#include "discord/private/_json.h"
#include "cutils.h"
#include "estr.h"

DISCORD_LOG_DEFINE_BASE();

esp_err_t discord_member_get(discord_handle_t client, char* guild_id, char* user_id, discord_member_t** out_member) {
    if(! client || ! guild_id || ! user_id || ! out_member) {
        DISCORD_LOGE("Invalid args");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = ESP_OK;
    discord_member_t* member = NULL;
    discord_api_response_t* res = NULL;
    
    if((err = dcapi_get(
        client,
        estr_cat("/guilds/", guild_id, "/members/", user_id),
        NULL,
        &res
    )) != ESP_OK) {
        return err;
    }

    if(dcapi_response_is_success(res) && res->data_len > 0) {
        member = discord_json_deserialize_(member, res->data, res->data_len);
    }

    dcapi_response_free(client, res);

    *out_member = member;
    return err;
}

static bool dc_member_permissions_calc(discord_handle_t client, discord_member_t* member, discord_role_t** roles, discord_role_len_t roles_len, uint64_t permissions) {
    discord_role_sort_list(roles, roles_len);

    uint64_t o_ring = strtoull(roles[0]->permissions, NULL, 10); // @everyone permissions

    if((o_ring & DISCORD_PERMISSION_ADMINISTRATOR) == DISCORD_PERMISSION_ADMINISTRATOR) { // @everyone has administrator
        return true;
    }

    bool in_ids = false;
    if(member->_roles_len > 0) {
        for(discord_role_len_t i = 1; i < roles_len; i++) {
            discord_role_is_in_ids_list(roles[i], member->roles, member->_roles_len, &in_ids);
            
            if(! in_ids) { // member does not have this role
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

esp_err_t discord_member_has_permissions(discord_handle_t client, discord_member_t* member, char* guild_id, uint64_t permissions, bool* out_result) {
    if(! client || ! member || ! guild_id || ! out_result) {
        DISCORD_LOGE("Invalid args");
        return ESP_ERR_INVALID_ARG;
    }

    discord_role_len_t len;
    discord_role_t** roles = NULL;
    esp_err_t err = discord_role_get_all(client, guild_id, &roles, &len);

    if(err != ESP_OK) {
        return err;
    }
    
    bool res = dc_member_permissions_calc(client, member, roles, len, permissions);
    cu_list_tfreex(roles, discord_role_len_t, len, discord_role_free);

    *out_result = res;
    return ESP_OK;
}

esp_err_t discord_member_has_role_name(discord_handle_t client, discord_member_t* member, const char* guild_id, const char* role_name, bool* out_result) {
    if(! client || ! member || ! guild_id || ! role_name || ! out_result) {
        DISCORD_LOGE("Invalid args");
        return ESP_ERR_INVALID_ARG;
    }

    discord_role_len_t len;
    discord_role_t** roles = NULL;
    
    esp_err_t err = discord_role_get_all(client, guild_id, &roles, &len);

    if(err != ESP_OK) {
        return err;
    }

    discord_role_t* required_role = NULL;

    for(discord_role_len_t i = 0; i < len; i++) {
        if(estr_eq(roles[i]->name, role_name)) {
            required_role = roles[i];
            break;
        }
    }

    if(required_role == NULL) { // role doesn't exist in guild
        cu_list_tfreex(roles, discord_role_len_t, len, discord_role_free);
        *out_result = false;
        return ESP_OK;
    }

    // role exist in guild, check if role is assigned to member
    bool result = false;
    for(discord_role_len_t i = 0; i < member->_roles_len; i++) {
        if(estr_eq(required_role->id, member->roles[i])) {
            result = true;
            break;
        }
    }

    *out_result = result;
    return ESP_OK;
}

void discord_member_free(discord_member_t* member) {
    if(!member)
        return;

    free(member->nick);
    free(member->permissions);
    cu_list_free(member->roles, member->_roles_len);
    free(member);
}