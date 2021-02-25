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

discord_session_t* discord_session_clone(discord_session_t* session);
void discord_session_free(discord_session_t* id);

#ifdef __cplusplus
}
#endif

#endif