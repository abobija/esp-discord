#include "discord/role.h"
#include "discord/private/_discord.h"
#include "discord/private/_api.h"
#include "discord/private/_json.h"
#include "estr.h"

DISCORD_LOG_DEFINE_BASE();

esp_err_t discord_role_get_all(discord_handle_t client, const char* guild_id, discord_role_t*** out_roles, discord_role_len_t* out_length) {
    if(! client || ! guild_id || ! out_roles || ! out_length) {
        DISCORD_LOGE("Invalid args");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = ESP_OK;
    discord_api_response_t* res = NULL;
    
    if((err = dcapi_get(client, estr_cat("/guilds/", guild_id, "/roles"), NULL, &res)) != ESP_OK) {
        DISCORD_LOGE("Fail to fetch roles");
        return err;
    }
    
    if(dcapi_response_is_success(res) && res->data_len > 0) {
        *out_roles = discord_json_list_deserialize_(role, res->data, res->data_len, out_length);
    }

    dcapi_response_free(client, res);

    return err;
}

esp_err_t discord_role_is_in_ids_list(discord_role_t* role, char** role_ids, discord_role_len_t role_ids_len, bool* out_result) {
    if(! role || ! role_ids || ! out_result) {
        return ESP_ERR_INVALID_ARG;
    }
    
    bool found = false;
    for(discord_role_len_t i = 0; i < role_ids_len; i++) {
        if(estr_eq(role_ids[i], role->id)) {
            found = true;
            break;
        }
    }

    *out_result = found;
    return ESP_OK;
}

static int dc_role_cmp(const void* role1, const void* role2) {
    return (*(discord_role_t**) role1)->position - (*(discord_role_t**) role2)->position;
}

esp_err_t discord_role_sort_list(discord_role_t** roles, discord_role_len_t len) {
    if(! roles) {
        return ESP_ERR_INVALID_ARG;
    }

    qsort(roles, len, sizeof(discord_role_t*), dc_role_cmp);

    return ESP_OK;
}

void discord_role_free(discord_role_t* role) {
    if(!role)
        return;

    free(role->id);
    free(role->name);
    free(role->permissions);
    free(role);
}