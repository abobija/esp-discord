#ifndef _DISCORD_MODELS_EMOJI_H_
#define _DISCORD_MODELS_EMOJI_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char* name;
} discord_emoji_t;

void discord_model_emoji_free(discord_emoji_t* emoji);

#ifdef __cplusplus
}
#endif

#endif