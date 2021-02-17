#ifndef _DISCORD_PRIVATE_API_H_
#define _DISCORD_PRIVATE_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "discord.h"

#define DCAPI_POST(strcater, serializer) ({ \
    char* _uri = strcater; \
    char* _json = serializer; \
    discord_api_response_t* _res = dcapi_post(client, _uri, _json); \
    free(_json); \
    free(_uri); \
    _res;\
})

typedef struct {
    int code;
    char* data;
} discord_api_response_t;

bool dcapi_response_is_success(discord_api_response_t* res);
void dcapi_response_free(discord_api_response_t* res);
discord_api_response_t* dcapi_post(discord_client_handle_t client, const char* uri, const char* data);
esp_err_t dcapi_close(discord_client_handle_t client);

#ifdef __cplusplus
}
#endif

#endif