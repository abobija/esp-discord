#ifndef _DISCORD_SESSION_H_
#define _DISCORD_SESSION_H_

#include "discord/user.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char* session_id;
    discord_user_t* user;
} discord_session_t;

#define discord_session_dump_log(LOG_FOO, TAG, session) \
    LOG_FOO(TAG, "Bot %s#%s connected (session_id=%s)", \
        session->user->username, \
        session->user->discriminator, \
        session->session_id \
    );

void discord_session_free(discord_session_t* id);

#ifdef __cplusplus
}
#endif

#endif