idf_component_register(
    SRCS src/discord.c
         src/discord/models.c
         src/discord/private/_discord.c
         src/discord/private/_models.c
         src/discord/private/_gateway.c
    INCLUDE_DIRS include
    REQUIRES esp_websocket_client json
)