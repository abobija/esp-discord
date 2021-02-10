idf_component_register(
    SRCS discord.c discord_gateway.c discord_gateway_heartbeat.c
    INCLUDE_DIRS .
    PRIV_REQUIRES esp_websocket_client json
)