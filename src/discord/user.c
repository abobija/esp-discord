#include "discord/private/_discord.h"
#include "discord/user.h"
#include "discord/guild.h"
#include "discord/private/_api.h"
#include "discord/private/_json.h"

DISCORD_LOG_DEFINE_BASE();

esp_err_t discord_user_get_my_guilds(discord_handle_t client, discord_guild_t*** out_guilds, int* out_length) {
    if(!client || !out_guilds || !out_length) {
        DISCORD_LOGE("Invalid args");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = ESP_OK;
    discord_api_response_t* res = NULL;
    
    if((err = dcapi_get(client, strdup("/users/@me/guilds"), NULL, &res)) != ESP_OK) {
        DISCORD_LOGE("Fail to fetch guilds");
        return err;
    }
    
    if(dcapi_response_is_success(res) && res->data_len > 0) {
        *out_guilds = discord_json_list_deserialize_(guild, res->data, res->data_len, out_length);
    }

    dcapi_response_free(client, res);

    return err;
}

void discord_user_free(discord_user_t* user) {
    if(!user)
        return;

    free(user->id);
    free(user->username);
    free(user->discriminator);
    free(user);
}