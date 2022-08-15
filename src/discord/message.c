#include "discord/message.h"
#include "discord/private/_discord.h"
#include "discord/private/_api.h"
#include "discord/private/_json.h"
#include "cutils.h"
#include "estr.h"

DISCORD_LOG_DEFINE_BASE();

static discord_api_multipart_t* discord_message_create_multipart_from_attachment(discord_attachment_t* attachment)
{
    return cu_ctor(discord_api_multipart_t,
        .name                  = estr_cat("files[", attachment->id, "]"),
        .mime_type             = strdup(attachment->content_type),
        .filename              = strdup(attachment->filename),
        .data                  = attachment->_data,
        .len                   = attachment->size,
        .data_should_be_freed  = attachment->_data_should_be_freed,
    );
}

esp_err_t discord_message_send(discord_handle_t client, discord_message_t* message, discord_message_t** out_result) {
    if(! client || ! message || ! message->channel_id) {
        DISCORD_LOGE("Invalid args");
        return ESP_ERR_INVALID_ARG;
    }

    discord_api_request_t* req = dcapi_create_request(
        estr_cat("/channels/", message->channel_id, "/messages"),
        discord_json_serialize(message)
    );

    for(uint8_t i = 0; i < message->_attachments_len; i++) {
        dcapi_add_multipart_to_request(discord_message_create_multipart_from_attachment(message->attachments[i]), req);
    }

    discord_api_response_t* res = NULL;
    esp_err_t err = dcapi_request(client, HTTP_METHOD_POST, req, &res);
    discord_api_request_free(req);

    if(err != ESP_OK) {
        return err;
    }

    if(! dcapi_response_is_success(res)) {
        dcapi_response_free(client, res);
        return ESP_ERR_INVALID_RESPONSE;
    }

    if(out_result) {
        if(res->data_len <= 0) {
            DISCORD_LOGW("Message sent but cannot return");
        } else {
            *out_result = discord_json_deserialize_(message, res->data, res->data_len);
        }
    }
    
    dcapi_response_free(client, res);
    return ESP_OK;
}

esp_err_t discord_message_react(discord_handle_t client, discord_message_t* message, const char* emoji) {
    if(!client || !message || !message->id || !message->channel_id) {
        DISCORD_LOGE("Invalid args");
        return ESP_FAIL;
    }

    char* _emoji = estr_url_encode(emoji);
    esp_err_t err = dcapi_put(client, estr_cat("/channels/", message->channel_id, "/messages/", message->id, "/reactions/", _emoji, "/@me"), NULL, NULL);
    free(_emoji);

    return err;
}

esp_err_t discord_message_download_attachment(discord_handle_t client, discord_message_t* message, uint8_t attachment_index, discord_download_handler_t download_handler, void* arg) {
    if(!client || !message || !message->attachments) {
        DISCORD_LOGE("Invalid args");
        return ESP_ERR_INVALID_ARG;
    }

    if(message->_attachments_len <= attachment_index) {
        DISCORD_LOGE("Message does not contain attachment with index %d", attachment_index);
        return ESP_ERR_INVALID_ARG;
    }

    discord_attachment_t* attach = message->attachments[attachment_index];

    discord_api_response_t* res = NULL;
    esp_err_t err = dcapi_download(client, attach->url, download_handler, &res, arg);
    if(err != ESP_OK) { return err; }
    err = dcapi_response_to_esp_err(res);
    dcapi_response_free(client, res);

    return err;
}

esp_err_t discord_message_word_parse(const char* word, discord_message_word_t** out_word) {
	if(!word || !out_word)
		return ESP_ERR_INVALID_ARG;
	
	discord_message_word_t* _word = cu_ctor(discord_message_word_t);
	int len = strlen(word);

	if(len < 4 || word[0] != '<' || word[len - 1] != '>') {
        goto _return;
    }
	
	if(word[1] == '@') {
		if(len > 3 && estrn_is_digit_only(_word->id = word + 2, _word->id_len = len - 3)) {
			_word->type = DISCORD_MESSAGE_WORD_USER;
		} else if(len > 4 && estrn_is_digit_only(_word->id = word + 3, _word->id_len = len - 4)) {
			if(word[2] == '!') {
				_word->type = DISCORD_MESSAGE_WORD_USER_NICKNAME;
			} else if(word[2] == '&') {
				_word->type = DISCORD_MESSAGE_WORD_ROLE;
			}
		}
	} else if(len > 3 && word[1] == '#' && estrn_is_digit_only(_word->id = word + 2, _word->id_len = len - 3)) {
		_word->type = DISCORD_MESSAGE_WORD_CHANNEL;
	} else if(len > 5 && estrn_chrcnt(word + 1, ':', len - 2) == 2) {
		if(word[1] == ':' &&
			(_word->name_len = strchr(_word->name = word + 2, ':') - word - 2) > 0 &&
			estrn_is_digit_only(_word->id = _word->name + _word->name_len + 1, _word->id_len = len - _word->name_len - 4)) {
				_word->type = DISCORD_MESSAGE_WORD_CUSTOM_EMOJI;
		} else if(len > 6 && word[1] == 'a' && word[2] == ':' &&
			(_word->name_len = strchr(_word->name = word + 3, ':') - word - 3) > 0 &&
			estrn_is_digit_only(_word->id = _word->name + _word->name_len + 1, _word->id_len = len - _word->name_len - 5)) {
				_word->type = DISCORD_MESSAGE_WORD_CUSTOM_EMOJI_ANIMATED;
		}
	}

	if(_word->type == DISCORD_MESSAGE_WORD_DEFAULT) {
		_word->name = _word->id = NULL;
		_word->name_len = _word->id_len = 0;
	}

_return:
    *out_word = _word;
	return ESP_OK;
}

esp_err_t discord_message_add_attachment(discord_message_t* message, discord_attachment_t* attachment)
{
    if(! message || ! attachment) {
        return ESP_ERR_INVALID_ARG;
    }

    message->attachments = realloc(message->attachments, ++message->_attachments_len * sizeof(discord_attachment_t*));
    int index = message->_attachments_len - 1;

    free(attachment->id);
    int length = snprintf(NULL, 0, "%d", index);
    attachment->id = malloc((length + 1) * sizeof(char));
    snprintf(attachment->id, length + 1, "%d", index);

    message->attachments[index] = attachment;
    return ESP_OK;
}

esp_err_t discord_message_add_embed(discord_message_t* message, discord_embed_t* embed)
{
    if(! message || ! embed) {
        return ESP_ERR_INVALID_ARG;
    }

    message->embeds = realloc(message->embeds, ++message->_embeds_len * sizeof(discord_embed_t*));
    message->embeds[message->_embeds_len - 1] = embed;

    return ESP_OK;
}

void discord_message_free(discord_message_t* message) {
    if(!message)
        return;
    
    free(message->id);
    free(message->content);
    free(message->channel_id);
    discord_user_free(message->author);
    free(message->guild_id);
    discord_member_free(message->member);
    cu_list_freex(message->attachments, message->_attachments_len, discord_attachment_free);
    cu_list_freex(message->embeds, message->_embeds_len, discord_embed_free);
    free(message);
}