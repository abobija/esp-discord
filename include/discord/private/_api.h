#ifndef _DISCORD_PRIVATE_API_H_
#define _DISCORD_PRIVATE_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_http_client.h"
#include "discord.h"

#define DCAPI_REQUEST_BOUNDARY "esp-discord"

#define DCAPI_POST(strcater, serializer, stream) ({ \
    char* _uri = strcater; \
    char* _json = serializer; \
    discord_api_response_t* _res = dcapi_post(client, _uri, _json, stream); \
    free(_json); \
    free(_uri); \
    _res;\
})

typedef struct {
    char* data;
    int len;
    const char* name;
    const char* mime_type;
} discord_api_multipart_t;

typedef struct {
    char* uri;
    discord_api_multipart_t** multiparts;
    uint8_t multiparts_len;
    bool disable_auto_uri_free;
    bool disable_auto_payload_free;
} discord_api_request_t;

typedef struct {
    int code;
    char* data;
    int data_len;
} discord_api_response_t;

bool dcapi_response_is_success(discord_api_response_t* res);
esp_err_t dcapi_response_to_esp_err(discord_api_response_t* res);
esp_err_t dcapi_response_free(discord_handle_t client, discord_api_response_t* res);
esp_err_t dcapi_download(discord_handle_t client, const char* url, discord_download_handler_t download_handler, discord_api_response_t** out_response, void* arg);
/**
 * GET request
 * 
 * @note data will be automatically freed
 */ 
esp_err_t dcapi_get(discord_handle_t client, char* uri, char* data, discord_api_response_t** out_response);
/**
 * POST request
 * 
 * @note data will be automatically freed
 */ 
esp_err_t dcapi_post(discord_handle_t client, char* uri, char* data, discord_api_response_t** out_response);
/**
 * PUT request
 * 
 * @note data will be automatically freed
 */
esp_err_t dcapi_put(discord_handle_t client, char* uri, char* data, discord_api_response_t** out_response);

esp_err_t dcapi_destroy(discord_handle_t client);

#ifdef __cplusplus
}
#endif

#endif