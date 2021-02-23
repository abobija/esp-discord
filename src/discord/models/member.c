#include "discord/models/member.h"
#include "esp_heap_caps.h"

void discord_member_free(discord_member_t* member) {
    if(!member)
        return;

    free(member->nick);
    free(member->permissions);
    if(member->roles) {
        for(uint8_t i = 0; i < member->_roles_len; i++) {
            free(member->roles[i]);
        }
        free(member->roles);
    }
    free(member);
}