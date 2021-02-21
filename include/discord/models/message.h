#ifndef _DISCORD_MODELS_MESSAGE_H_
#define _DISCORD_MODELS_MESSAGE_H_

#include "discord/models/user.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char* id;
    char* content;
    char* channel_id;
    discord_user_t* author;
} discord_message_t;

void discord_message_free(discord_message_t* message);

#ifdef __cplusplus
}
#endif

#endif