#include "discord/private/_discord.h"
#include "discord/private/_api.h"
#include "discord/private/_json.h"
#include "estr.h"

#include "discord/guild.h"

DISCORD_LOG_DEFINE_BASE();

esp_err_t discord_guild_get_channels(discord_handle_t client, discord_guild_t* guild, discord_channel_t*** out_channels, int* out_length) {
    if(!client || !guild || !out_channels || !out_length) {
        DISCORD_LOGE("Invalid args");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = ESP_OK;
    discord_api_response_t* res = NULL;
    
    if((err = dcapi_get(client, estr_cat("/guilds/", guild->id, "/channels"), NULL, &res)) != ESP_OK) {
        DISCORD_LOGE("Fail to fetch channels");
        return err;
    }
    
    if(dcapi_response_is_success(res) && res->data_len > 0) {
        *out_channels = discord_json_list_deserialize_(channel, res->data, res->data_len, out_length);
    }
    
    dcapi_response_free(client, res);

    return err;
}

void discord_guild_free(discord_guild_t* guild) {
    if(!guild)
        return;

    free(guild->id);
    free(guild->name);
    free(guild->permissions);
    free(guild);
}