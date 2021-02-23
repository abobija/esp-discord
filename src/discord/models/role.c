#include "discord/models/role.h"
#include "esp_heap_caps.h"

void discord_role_free(discord_role_t* role) {
    if(!role)
        return;

    free(role->id);
    free(role->name);
    free(role->permissions);
    free(role);
}

void discord_role_list_free(discord_role_t** roles, uint8_t len) {
    if(!roles)
        return;
    
    for(uint8_t i = 0; i < len; i++) {
        discord_role_free(roles[i]);
    }

    free(roles);
}