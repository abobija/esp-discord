#include "discord/models/message.h"
#include "esp_heap_caps.h"

void discord_model_message_free(discord_message_t* message) {
    if(message == NULL)
        return;

    free(message->id);
    free(message->content);
    free(message->channel_id);
    discord_model_user_free(message->author);
    free(message);
}