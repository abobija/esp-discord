#include "discord/session.h"
#include "string.h"
#include "esp_heap_caps.h"

discord_session_t* discord_session_ctor(char* id, discord_user_t* user) {
    discord_session_t* session = calloc(1, sizeof(discord_session_t));

    session->session_id = id;
    session->user = user;

    return session;
}

discord_session_t* discord_session_clone(discord_session_t* session) {
    if(!session)
        return NULL;

    return discord_session_ctor(
        strdup(session->session_id),
        discord_user_clone(session->user)
    );
}

void discord_session_free(discord_session_t* session) {
    if(!session)
        return;

    discord_user_free(session->user);
    free(session->session_id);
    free(session);
}