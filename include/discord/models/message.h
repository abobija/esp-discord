#ifndef _DISCORD_MODELS_MESSAGE_H_
#define _DISCORD_MODELS_MESSAGE_H_

#include "discord/models/user.h"

typedef struct {
    char* id;
    char* content;
    char* channel_id;
    discord_user_t* author;
} discord_message_t;

#endif