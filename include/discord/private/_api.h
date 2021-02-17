#ifndef _DISCORD_PRIVATE_API_H_
#define _DISCORD_PRIVATE_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "discord.h"

#define DCAPI_POST(strcater, serializer) { \
    char* uri = strcater; \
    char* json = serializer; \
    dcapi_post(client, uri, json); \
    free(json); \
    free(uri); \
}

esp_err_t dcapi_post(discord_client_handle_t client, const char* uri, const char* data);
esp_err_t dcapi_close(discord_client_handle_t client);

#ifdef __cplusplus
}
#endif

#endif