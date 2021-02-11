#include "string.h"
#include "esp_heap_caps.h"
#include "cJSON.h"
#include "discord_models.h"

discord_gateway_identify_properties_t* discord_model_gateway_identify_properties(const char* $os, const char* $browser, const char* $device) {
    discord_gateway_identify_properties_t* props = calloc(1, sizeof(discord_gateway_identify_properties_t));

    props->$os = strdup($os);
    props->$browser = strdup($browser);
    props->$device = strdup($device);

    return props;
}

cJSON* discord_model_gateway_identify_properties_to_cjson(discord_gateway_identify_properties_t* properties) {
    cJSON* root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "$os", properties->$os);
    cJSON_AddStringToObject(root, "$browser", properties->$browser);
    cJSON_AddStringToObject(root, "$device", properties->$device);

    return root;
}

void discord_model_gateway_identify_properties_free(discord_gateway_identify_properties_t* properties) {
    free(properties->$os);
    free(properties->$browser);
    free(properties->$device);
    free(properties);
}

discord_gateway_identify_t* discord_model_gateway_identify(const char* token, int intents, discord_gateway_identify_properties_t* properties) {
    discord_gateway_identify_t* identify = calloc(1, sizeof(discord_gateway_identify_t));

    identify->token = strdup(token);
    identify->intents = intents;
    identify->properties = properties;

    return identify;
}

cJSON* discord_model_gateway_identify_to_cjson(discord_gateway_identify_t* identify) {
    cJSON* root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "token", identify->token);
    cJSON_AddNumberToObject(root, "intents", identify->intents);
    cJSON_AddItemToObject(root, "properties", discord_model_gateway_identify_properties_to_cjson(identify->properties));

    return root;
}

void discord_model_gateway_identify_free(discord_gateway_identify_t* identify) {
    free(identify->token);
    discord_model_gateway_identify_properties_free(identify->properties);
    free(identify);
}

discord_gateway_session_user_t* discord_model_gateway_session_user_from_cjson(cJSON* root) {
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

discord_gateway_session_t* discord_model_gateway_session_from_cjson(cJSON* root) {
    discord_gateway_session_t* id = calloc(1, sizeof(discord_gateway_session_t));

    id->session_id = strdup(cJSON_GetObjectItem(root, "session_id")->valuestring);
    id->user = discord_model_gateway_session_user_from_cjson(cJSON_GetObjectItem(root, "user"));

    return id;
}

void discord_model_gateway_session_free(discord_gateway_session_t* id) {
    discord_model_gateway_session_user_free(id->user);
    free(id->session_id);
    free(id);
}

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
    free(message->id);
    free(message->content);
    free(message->channel_id);
    discord_model_user_free(message->author);
    free(message);
}