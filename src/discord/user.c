#include "discord/private/_discord.h"
#include "discord/user.h"
#include "esp_heap_caps.h"
#include "discord/utils.h"

void discord_user_free(discord_user_t* user) {
    if(!user)
        return;

    free(user->id);
    free(user->username);
    free(user->discriminator);
    free(user);
}