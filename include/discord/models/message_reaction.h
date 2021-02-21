#ifndef _DISCORD_MODELS_MESSAGE_REACTION_H_
#define _DISCORD_MODELS_MESSAGE_REACTION_H_

#include "discord/models/emoji.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char* user_id;
    char* message_id;
    char* channel_id;
    discord_emoji_t* emoji;
} discord_message_reaction_t;

void discord_model_message_reaction_free(discord_message_reaction_t* reaction);

#ifdef __cplusplus
}
#endif

#endif