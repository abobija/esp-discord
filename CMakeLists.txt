set(CERTS "")

if(NOT CMAKE_BUILD_EARLY_EXPANSION AND EXISTS config AND NOT CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY)
    list(APPEND CERTS cert/gateway.pem cert/api.pem)

    if(NOT EXISTS ${COMPONENT_DIR}/cert/gateway.pem OR NOT EXISTS ${COMPONENT_DIR}/cert/api.pem)
        execute_process(COMMAND ${COMPONENT_DIR}/certgen.sh RESULT_VARIABLE CERTGEN_RESULT)

        if(NOT CERTGEN_RESULT EQUAL "0")
            message(FATAL_ERROR "Failed to generate Discord certificates\n${CERTGEN_RESULT}")
        endif()
    endif()
endif()

idf_component_register(
    SRCS src/helpers/estr.c
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
         src/discord/guild.c
         src/discord/channel.c
         src/discord/role.c
         src/discord/attachment.c
         src/discord/embed.c
         src/discord/voice_state.c
         src/discord.c
         src/discord_ota.c
    INCLUDE_DIRS include include/helpers
    REQUIRES json esp_websocket_client esp_http_client
    PRIV_REQUIRES app_update nvs_flash
    EMBED_TXTFILES ${CERTS}
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)