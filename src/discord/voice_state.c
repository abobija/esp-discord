#include "discord/voice_state.h"

void discord_voice_state_free(discord_voice_state_t* voice_state) {
    if(!voice_state)
        return;

    free(voice_state->guild_id);
    free(voice_state->channel_id);
    free(voice_state->user_id);
    discord_member_free(voice_state->member);
    free(voice_state);
}