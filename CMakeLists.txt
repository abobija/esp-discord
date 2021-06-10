set(CERTS "")

if(NOT CMAKE_BUILD_EARLY_EXPANSION AND EXISTS config AND NOT CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY)
    list(APPEND CERTS cert/gateway.pem cert/api.pem)

    if(NOT EXISTS ${COMPONENT_DIR}/cert/gateway.pem OR NOT EXISTS ${COMPONENT_DIR}/cert/api.pem)
        string(CONCAT certs_missing_message
            "\n ****************************************************************************"
            "\n "
            "\n Discord certificates missing. To generate certs please run next script:"
            "\n ${COMPONENT_DIR}/certgen.sh"
            "\n "
            "\n On Windows script should be automatically open in Git-Bash by double-click on it."
            "\n If not, open Git-Bash (on Windows) and run script manually."
            "\n "
            "\n ****************************************************************************"
        )

        string(ASCII 27 ESCAPE)
        message(FATAL_ERROR "${ESCAPE}[33m ${certs_missing_message} ${ESCAPE}[0m")
    endif()
endif()

idf_component_register(
    SRCS cutils/src/estr.c
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
         src/discord/voice_state.c
         src/discord.c
         src/discord_ota.c
    INCLUDE_DIRS include cutils/include
    REQUIRES json esp_websocket_client esp_http_client
    PRIV_REQUIRES app_update nvs_flash
    EMBED_TXTFILES ${CERTS}
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)