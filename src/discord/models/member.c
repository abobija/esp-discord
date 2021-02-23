#include "discord/models/member.h"
#include "esp_heap_caps.h"

void discord_member_free(discord_member_t* member) {
    if(!member)
        return;

    free(member->nick);
    free(member->permissions);
    free(member);
}