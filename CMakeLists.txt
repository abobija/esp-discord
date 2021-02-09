idf_component_register(
    SRCS "discord.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_websocket_client
)