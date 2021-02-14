idf_component_register(
    SRCS src/discord.c src/discord_models_private.c src/discord_models.c
    INCLUDE_DIRS include
    REQUIRES json
    PRIV_REQUIRES esp_websocket_client
)