#ifndef _DISCORD_MODELS_MESSAGE_H_
#define _DISCORD_MODELS_MESSAGE_H_

#include "discord/models/user.h"
#include "discord/models/member.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char* id;
    char* content;
    char* channel_id;
    discord_user_t* author;
    char* guild_id;
    discord_member_t* member;
} discord_message_t;

void discord_message_free(discord_message_t* message);

#ifdef __cplusplus
}
#endif

#endif