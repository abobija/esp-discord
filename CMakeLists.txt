idf_component_register(
    SRCS discord.c
         discord_gateway.c
         discord_models.c
    INCLUDE_DIRS .
    REQUIRES esp_websocket_client
             json
)