#include "discord/members.h"
#include "discord/private/_discord.h"
#include "discord/private/_api.h"
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
        member = discord_member_deserialize(res->data, res->data_len);
    }

    dcapi_response_free(client, res);

    return member;
}