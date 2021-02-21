#include "discord/models/session.h"
#include "esp_heap_caps.h"

void discord_session_free(discord_session_t* session) {
    if(!session)
        return;

    discord_user_free(session->user);
    free(session->session_id);
    free(session);
}