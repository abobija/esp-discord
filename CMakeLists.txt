set(CERTS "")

if(NOT CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY)
    list(APPEND CERTS cert/gateway.pem cert/api.pem)

    if(NOT EXISTS ${COMPONENT_DIR}/cert/gateway.pem OR NOT EXISTS ${COMPONENT_DIR}/cert/api.pem)
        string(CONCAT certs_missing_message
            "\n ****************************************************************************"
            "\n Discord certificates missing. To generate certs please run next script:"
            "\n ${COMPONENT_DIR}/certgen.sh"
            "\n "
            "\n On Windows, script should be automatically open in Git-Bash by double-click on it."
            "\n If not, open Git-Bash manually (on Windows) and run script."
            "\n "
            "\n Or, to skip certification, enable next option in \"idf.py menuconfig\":"
            "\n     Component Config"
            "\n         -> ESP-TLS"
            "\n         -> Allow potentially insecure options"
            "\n         -> Skip server certificate verification by default"
            "\n ****************************************************************************"
        )

        string(ASCII 27 ESCAPE)
        message(FATAL_ERROR "${ESCAPE}[33m ${certs_missing_message} ${ESCAPE}[0m")
    endif()
endif()

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
    EMBED_TXTFILES ${CERTS}
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)