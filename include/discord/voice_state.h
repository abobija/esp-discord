#ifndef _DISCORD_VOICE_STATE_H_
#define _DISCORD_VOICE_STATE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "discord.h"
#include "discord/member.h"

typedef struct {
    char* guild_id;            /*!< The guild id this voice state is for */
    char* channel_id;          /*!< The channel id this user is connected to */
    char* user_id;             /*!< The user id this voice state is for */
    discord_member_t* member;  /*!< The guild member this voice state is for */
    bool deaf;                 /*!< Whether this user is deafened by the server */
    bool mute;                 /*!< Whether this user is muted by the server */
    bool self_deaf;            /*!< Whether this user is locally deafened */
    bool self_mute;            /*!< Whether this user is locally muted */
} discord_voice_state_t;

void discord_voice_state_free(discord_voice_state_t* voice_state);

#ifdef __cplusplus
}
#endif

#endif