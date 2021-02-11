#include "string.h"
#include "esp_heap_caps.h"
#include "cJSON.h"
#include "discord_models.h"

discord_gateway_session_user_t* discord_model_gateway_session_user(cJSON* root) {
    discord_gateway_session_user_t* user = calloc(1, sizeof(discord_gateway_session_user_t));

    user->id = strdup(cJSON_GetObjectItem(root, "id")->valuestring);
    user->username = strdup(cJSON_GetObjectItem(root, "username")->valuestring);
    user->discriminator = strdup(cJSON_GetObjectItem(root, "discriminator")->valuestring);

    return user;
}

void discord_model_gateway_session_user_free(discord_gateway_session_user_t* user) {
    free(user->id);
    free(user->username);
    free(user->discriminator);
    free(user);
}

discord_gateway_session_t* discord_model_gateway_session(cJSON* root) {
    discord_gateway_session_t* id = calloc(1, sizeof(discord_gateway_session_t));

    id->session_id = strdup(cJSON_GetObjectItem(root, "session_id")->valuestring);
    id->user = discord_model_gateway_session_user(cJSON_GetObjectItem(root, "user"));

    return id;
}

void discord_model_gateway_session_free(discord_gateway_session_t* id) {
    discord_model_gateway_session_user_free(id->user);
    free(id->session_id);
    free(id);
}

discord_user_t* discord_model_user(cJSON* root) {
    discord_user_t* user = calloc(1, sizeof(discord_user_t));

    user->id = strdup(cJSON_GetObjectItem(root, "id")->valuestring);
    user->username = strdup(cJSON_GetObjectItem(root, "username")->valuestring);
    user->discriminator = strdup(cJSON_GetObjectItem(root, "discriminator")->valuestring);

    cJSON* bot = cJSON_GetObjectItem(root, "bot");
    user->bot = bot && bot->valueint;

    return user;
}

void discord_model_user_free(discord_user_t* user) {
    free(user->id);
    free(user->username);
    free(user->discriminator);
    free(user);
}