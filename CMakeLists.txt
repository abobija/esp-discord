idf_component_register(
    SRCS src/discord/utils.c
         src/discord/private/_models.c
         src/discord/private/_gateway.c
         src/discord/private/_api.c
         src/discord/private/_json.c
         src/discord/user.c
         src/discord/session.c
         src/discord/member.c
         src/discord/message.c
         src/discord/emoji.c
         src/discord/message_reaction.c
         src/discord/role.c
         src/discord/attachment.c
         src/discord.c
    INCLUDE_DIRS include
    REQUIRES json esp_websocket_client esp_http_client
)