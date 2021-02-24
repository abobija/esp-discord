#include "discord/private/_discord.h"
#include "discord/user.h"
#include "esp_heap_caps.h"

discord_user_t* discord_user_ctor(char* id, bool bot, char* username, char* discriminator) {
    discord_user_t* user = calloc(1, sizeof(discord_user_t));

    user->id = id;
    user->bot = bot;
    user->username = username;
    user->discriminator = discriminator;

    return user;
}

discord_user_t* discord_user_clone(discord_user_t* user) {
    if(!user)
        return NULL;

    return discord_user_ctor(
        STRDUP(user->id),
        user->bot,
        STRDUP(user->username),
        STRDUP(user->discriminator)
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