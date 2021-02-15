#include "discord/private/_discord.h"

uint64_t dc_tick_ms(void) {
    return esp_timer_get_time() / 1000;
}