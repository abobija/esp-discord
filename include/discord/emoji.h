#ifndef _DISCORD_EMOJI_H_
#define _DISCORD_EMOJI_H_

#ifdef __cplusplus
extern "C" {
#endif

#define DISCORD_EMOJI_THUMBSUP          "👍"
#define DISCORD_EMOJI_THUMBSDOWN        "👎"
#define DISCORD_EMOJI_WAVE              "👋"
#define DISCORD_EMOJI_SMILE             "😄"
#define DISCORD_EMOJI_SMIRK             "😏"
#define DISCORD_EMOJI_UNAMUSED          "😒"
#define DISCORD_EMOJI_SUNGLASSES        "😎"
#define DISCORD_EMOJI_CONFUSED          "😕"
#define DISCORD_EMOJI_WHITE_CHECK_MARK  "✅"
#define DISCORD_EMOJI_X                 "❌"

typedef struct {
    char* name;
} discord_emoji_t;

void discord_emoji_free(discord_emoji_t* emoji);

#ifdef __cplusplus
}
#endif

#endif