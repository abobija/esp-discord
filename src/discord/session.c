#include "discord/session.h"
#include "discord/private/_discord.h"

esp_err_t discord_session_get_current(discord_handle_t client, const discord_session_t** out_session) {
    if(!client || !out_session) {
        return ESP_ERR_INVALID_ARG;
    }

    *out_session = client->session;
    return ESP_OK;
}

void discord_session_free(discord_session_t* session) {
    if(!session)
        return;

    discord_user_free(session->user);
    free(session->session_id);
    free(session);
}