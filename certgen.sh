#!/bin/bash

discord_cert_gen() {
    echo "" | openssl s_client -showcerts -connect $1:443 2> /dev/null | sed -n "1,/Root/d; /BEGIN/,/END/p" | openssl x509 -outform PEM > "$2"
}

discord_certs() {
    local DIRNAME=$(dirname "${BASH_SOURCE}")
    local CERT_DIR="${DIRNAME}/cert"
    
    mkdir "${CERT_DIR}" 2> /dev/null
    
    echo "Generating gateway certificate..."
    discord_cert_gen gateway.discord.gg "${CERT_DIR}/gateway.pem"

    if [[ $? == 0 ]]; then
        echo "Generated."
        echo "Generating api certificate..."
        discord_cert_gen discord.com "${CERT_DIR}/api.pem"
    fi

    if [[ $? == 0 ]]; then
        echo "Generated."
    fi

    echo "DONE."
}

discord_certs