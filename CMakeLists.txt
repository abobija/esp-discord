idf_component_register(
    SRCS discord.c
         discord_gateway.c
    INCLUDE_DIRS .
    REQUIRES esp_websocket_client
             json
)