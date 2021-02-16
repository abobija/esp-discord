#include "discord/private/_discord.h"
#include "esp_heap_caps.h"
#include "cJSON.h"
#include "esp_log.h"
#include "discord.h"
#include "discord/models.h"

void discord_model_user_free(discord_user_t* user) {
    if(user == NULL)
        return;

    free(user->id);
    free(user->username);
    free(user->discriminator);
    free(user);
}

discord_message_t* discord_model_message(const char* id, const char* content, const char* channel_id, discord_user_t* author) {
    discord_message_t* message = calloc(1, sizeof(discord_message_t));

    message->id = STRDUP(id);
    message->content = STRDUP(content);
    message->channel_id = STRDUP(channel_id);
    message->author = author;

    return message;
}

void discord_model_message_free(discord_message_t* message) {
    if(message == NULL)
        return;

    free(message->id);
    free(message->content);
    free(message->channel_id);
    discord_model_user_free(message->author);
    free(message);
}