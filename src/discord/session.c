#include "discord/session.h"
#include "string.h"
#include "esp_heap_caps.h"
#include "discord/utils.h"

discord_session_t* discord_session_clone(discord_session_t* session) {
    if(!session)
        return NULL;

    return discord_ctor(discord_session_t,
        .session_id = strdup(session->session_id),
        .user = discord_user_clone(session->user)
    );
}

void discord_session_free(discord_session_t* session) {
    if(!session)
        return;

    discord_user_free(session->user);
    free(session->session_id);
    free(session);
}