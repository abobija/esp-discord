#include "discord/models/user.h"
#include "esp_heap_caps.h"

void discord_user_free(discord_user_t* user) {
    if(user == NULL)
        return;

    free(user->id);
    free(user->username);
    free(user->discriminator);
    free(user);
}