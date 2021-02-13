idf_component_register(
    SRCS discord.c discord_models_private.c discord_models.c
    INCLUDE_DIRS .
    REQUIRES json
    PRIV_REQUIRES esp_websocket_client
)