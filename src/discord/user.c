#include "discord/private/_discord.h"
#include "discord/user.h"
#include "esp_heap_caps.h"
#include "discord/utils.h"

discord_user_t* discord_user_clone(discord_user_t* user) {
    if(!user)
        return NULL;

    return discord_ctor(discord_user_t,
        .id = STRDUP(user->id),
        .bot = user->bot,
        .username = STRDUP(user->username),
        .discriminator = STRDUP(user->discriminator)
    );
}

void discord_user_free(discord_user_t* user) {
    if(!user)
        return;

    free(user->id);
    free(user->username);
    free(user->discriminator);
    free(user);
}