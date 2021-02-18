idf_component_register(
    SRCS src/discord.c
         src/discord/messages.c
         src/discord/models/user.c
         src/discord/models/message.c
         src/discord/private/_discord.c
         src/discord/private/_models.c
         src/discord/private/_gateway.c
         src/discord/private/_api.c
    INCLUDE_DIRS include
    REQUIRES json esp_websocket_client esp_http_client
)