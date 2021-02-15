idf_component_register(
    SRCS src/discord.c
         src/discord/models.c
         src/discord/private/models.c
    INCLUDE_DIRS include
    REQUIRES json
    PRIV_REQUIRES esp_websocket_client
)