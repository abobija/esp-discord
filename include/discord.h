#ifndef _DISCORD_H_
#define _DISCORD_H_

#include "esp_err.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

// intents

#define DISCORD_INTENT_GUILDS                    (1 << 0)
#define DISCORD_INTENT_GUILD_MEMBERS             (1 << 1)
#define DISCORD_INTENT_GUILD_BANS                (1 << 2)
#define DISCORD_INTENT_GUILD_EMOJIS              (1 << 3)
#define DISCORD_INTENT_GUILD_INTEGRATIONS        (1 << 4)
#define DISCORD_INTENT_GUILD_WEBHOOKS            (1 << 5)
#define DISCORD_INTENT_GUILD_INVITES             (1 << 6)
#define DISCORD_INTENT_GUILD_VOICE_STATES        (1 << 7)
#define DISCORD_INTENT_GUILD_PRESENCES           (1 << 8)
#define DISCORD_INTENT_GUILD_MESSAGES            (1 << 9)
#define DISCORD_INTENT_GUILD_MESSAGE_REACTIONS   (1 << 10)
#define DISCORD_INTENT_GUILD_MESSAGE_TYPING      (1 << 11)
#define DISCORD_INTENT_DIRECT_MESSAGES           (1 << 12)
#define DISCORD_INTENT_DIRECT_MESSAGE_REACTIONS  (1 << 13)
#define DISCORD_INTENT_DIRECT_MESSAGE_TYPING     (1 << 14)

// permissions

#define DISCORD_PERMISSION_CREATE_INSTANT_INVITE  0x00000001ULL  /*!< Allows creation of instant invites */
#define DISCORD_PERMISSION_KICK_MEMBERS           0x00000002ULL  /*!< Allows kicking members */
#define DISCORD_PERMISSION_BAN_MEMBERS            0x00000004ULL  /*!< Allows banning members */
#define DISCORD_PERMISSION_ADMINISTRATOR          0x00000008ULL  /*!< Allows all permissions and bypasses channel permission overwrites */
#define DISCORD_PERMISSION_MANAGE_CHANNELS        0x00000010ULL  /*!< Allows management and editing of channels */
#define DISCORD_PERMISSION_MANAGE_GUILD           0x00000020ULL  /*!< Allows management and editing of the guild */
#define DISCORD_PERMISSION_ADD_REACTIONS          0x00000040ULL  /*!< Allows for the addition of reactions to messages */
#define DISCORD_PERMISSION_VIEW_AUDIT_LOG         0x00000080ULL  /*!< Allows for viewing of audit logs */
#define DISCORD_PERMISSION_PRIORITY_SPEAKER       0x00000100ULL  /*!< Allows for using priority speaker in a voice channel */
#define DISCORD_PERMISSION_STREAM                 0x00000200ULL  /*!< Allows the user to go live */
#define DISCORD_PERMISSION_VIEW_CHANNEL           0x00000400ULL  /*!< Allows guild members to view a channel, which includes reading messages in text channels */
#define DISCORD_PERMISSION_SEND_MESSAGES          0x00000800ULL  /*!< Allows for sending messages in a channel */
#define DISCORD_PERMISSION_SEND_TTS_MESSAGES      0x00001000ULL  /*!< Allows for sending of /tts messages */
#define DISCORD_PERMISSION_MANAGE_MESSAGES        0x00002000ULL  /*!< Allows for deletion of other users messages */
#define DISCORD_PERMISSION_EMBED_LINKS            0x00004000ULL  /*!< Links sent by users with this permission will be auto-embedded */
#define DISCORD_PERMISSION_ATTACH_FILES           0x00008000ULL  /*!< Allows for uploading images and files */
#define DISCORD_PERMISSION_READ_MESSAGE_HISTORY   0x00010000ULL  /*!< Allows for reading of message history */
#define DISCORD_PERMISSION_MENTION_EVERYONE       0x00020000ULL  /*!< Allows for using the @everyone tag to notify all users in a channel, and the @here tag to notify all online users in a channel */
#define DISCORD_PERMISSION_USE_EXTERNAL_EMOJIS    0x00040000ULL  /*!< Allows the usage of custom emojis from other servers */
#define DISCORD_PERMISSION_VIEW_GUILD_INSIGHTS    0x00080000ULL  /*!< Allows for viewing guild insights */
#define DISCORD_PERMISSION_CONNECT                0x00100000ULL  /*!< Allows for joining of a voice channel */
#define DISCORD_PERMISSION_SPEAK                  0x00200000ULL  /*!< Allows for speaking in a voice channel */
#define DISCORD_PERMISSION_MUTE_MEMBERS           0x00400000ULL  /*!< Allows for muting members in a voice channel */
#define DISCORD_PERMISSION_DEAFEN_MEMBERS         0x00800000ULL  /*!< Allows for deafening of members in a voice channel */
#define DISCORD_PERMISSION_MOVE_MEMBERS           0x01000000ULL  /*!< Allows for moving of members between voice channels */
#define DISCORD_PERMISSION_USE_VAD                0x02000000ULL  /*!< Allows for using voice-activity-detection in a voice channel */
#define DISCORD_PERMISSION_CHANGE_NICKNAME        0x04000000ULL  /*!< Allows for modification of own nickname */
#define DISCORD_PERMISSION_MANAGE_NICKNAMES       0x08000000ULL  /*!< Allows for modification of other users nicknames */
#define DISCORD_PERMISSION_MANAGE_ROLES           0x10000000ULL  /*!< Allows management and editing of roles */
#define DISCORD_PERMISSION_MANAGE_WEBHOOKS        0x20000000ULL  /*!< Allows management and editing of webhooks */
#define DISCORD_PERMISSION_MANAGE_EMOJIS          0x40000000ULL  /*!< Allows management and editing of emojis */

typedef struct {
    char* token;
    int intents;
    int buffer_size;
} discord_client_config_t;

typedef struct discord_client* discord_client_handle_t;

ESP_EVENT_DECLARE_BASE(DISCORD_EVENTS);

typedef enum {
    DISCORD_EVENT_UNKNOWN = -2,
    DISCORD_EVENT_ANY = ESP_EVENT_ANY_ID,
    DISCORD_EVENT_NONE,
    DISCORD_EVENT_READY,                       /*<! This event will never be fired. Use DISCORD_EVENT_CONNECTED instead. */
    DISCORD_EVENT_CONNECTED,
    DISCORD_EVENT_MESSAGE_RECEIVED,
    DISCORD_EVENT_MESSAGE_UPDATED,
    DISCORD_EVENT_MESSAGE_DELETED,
    DISCORD_EVENT_MESSAGE_REACTION_ADDED,
    DISCORD_EVENT_MESSAGE_REACTION_REMOVED
} discord_event_t;

typedef void* discord_event_data_ptr_t;

typedef struct {
    discord_client_handle_t client;
    discord_event_data_ptr_t ptr;
} discord_event_data_t;

discord_client_handle_t discord_create(const discord_client_config_t* config);
esp_err_t discord_login(discord_client_handle_t client);
esp_err_t discord_register_events(discord_client_handle_t client, discord_event_t event, esp_event_handler_t event_handler, void* event_handler_arg);
esp_err_t discord_logout(discord_client_handle_t client);
esp_err_t discord_destroy(discord_client_handle_t client);

#ifdef __cplusplus
}
#endif

#endif