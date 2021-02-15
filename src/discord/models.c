#include "string.h"
#include "esp_heap_caps.h"
#include "cJSON.h"
#include "esp_log.h"
#include "discord.h"
#include "discord/models.h"

discord_user_t* discord_model_user_from_cjson(cJSON* root) {
    discord_user_t* user = calloc(1, sizeof(discord_user_t));

    user->id = strdup(cJSON_GetObjectItem(root, "id")->valuestring);
    user->username = strdup(cJSON_GetObjectItem(root, "username")->valuestring);
    user->discriminator = strdup(cJSON_GetObjectItem(root, "discriminator")->valuestring);

    cJSON* bot = cJSON_GetObjectItem(root, "bot");
    user->bot = bot && bot->valueint;

    return user;
}

void discord_model_user_free(discord_user_t* user) {
    if(user == NULL)
        return;

    free(user->id);
    free(user->username);
    free(user->discriminator);
    free(user);
}

discord_message_t* discord_model_message_from_cjson(cJSON* root) {
    discord_message_t* message = calloc(1, sizeof(discord_message_t));

    message->id = strdup(cJSON_GetObjectItem(root, "id")->valuestring);
    message->content = strdup(cJSON_GetObjectItem(root, "content")->valuestring);
    message->channel_id = strdup(cJSON_GetObjectItem(root, "channel_id")->valuestring);
    message->author = discord_model_user_from_cjson(cJSON_GetObjectItem(root, "author"));

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