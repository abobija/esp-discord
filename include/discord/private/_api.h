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

// TODO: Maybe discord_api_multipart_t type needs to be deleted and use discord_attachment_t type instead?
//       That will remove headache for making multiparts from attachment, and props are the same

typedef struct {
    char* data;
    int len;
    char* name;
    char* filename;
    char* mime_type;
    bool data_should_be_freed; /*<! Set to true if data should be freed by discord_api_multipart_free function */
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
esp_err_t dcapi_request(discord_handle_t client, esp_http_client_method_t method, discord_api_request_t* request, discord_api_response_t** out_response);
esp_err_t dcapi_download(discord_handle_t client, const char* url, discord_download_handler_t download_handler, discord_api_response_t** out_response, void* arg);
esp_err_t dcapi_add_multipart_to_request(discord_api_multipart_t* multipart, discord_api_request_t* request);
void discord_api_request_free(discord_api_request_t* request);
/**
 * @brief Helper function for creating new request.
 *        Function will automatically add payload as multipart of request
 */
discord_api_request_t* dcapi_create_request(char* uri, char* payload);
/**
 * @brief GET request
 * 
 * @note data will be automatically freed
 */ 
esp_err_t dcapi_get(discord_handle_t client, char* uri, char* data, discord_api_response_t** out_response);
/**
 * @brief POST request
 * 
 * @note data will be automatically freed
 */ 
esp_err_t dcapi_post(discord_handle_t client, char* uri, char* data, discord_api_response_t** out_response);
/**
 * @brief PUT request
 * 
 * @note data will be automatically freed
 */
esp_err_t dcapi_put(discord_handle_t client, char* uri, char* data, discord_api_response_t** out_response);

esp_err_t dcapi_destroy(discord_handle_t client);

#ifdef __cplusplus
}
#endif

#endif