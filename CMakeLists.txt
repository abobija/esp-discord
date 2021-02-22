idf_component_register(
    SRCS src/discord/private/_models.c
         src/discord/private/_gateway.c
         src/discord/private/_api.c
         src/discord.c
         src/discord/utils.c
         src/discord/messages.c
         src/discord/models/user.c
         src/discord/models/session.c
         src/discord/models/message.c
         src/discord/models/emoji.c
         src/discord/models/message_reaction.c
    INCLUDE_DIRS include
    REQUIRES json esp_websocket_client esp_http_client
)