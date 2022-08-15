#ifndef _DISCORD_H_
#define _DISCORD_H_

#include "esp_err.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DISCORD_VER_MAJOR 1
#define DISCORD_VER_MINOR 3
#define DISCORD_VER_STR_(x) #x
#define DISCORD_VER_STR(x) DISCORD_VER_STR_(x)
#define DISCORD_VER_STRING DISCORD_VER_STR(DISCORD_VER_MAJOR) "." DISCORD_VER_STR(DISCORD_VER_MINOR)

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
#define DISCORD_INTENT_MESSAGE_CONTENT           (1 << 15)

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

// colors (taken from https://bit.ly/3QMMXQr)

#define DISCORD_COLOR_DEFAULT              (0)
#define DISCORD_COLOR_AQUA                 (1752220)
#define DISCORD_COLOR_DARK_AQUA            (1146986)
#define DISCORD_COLOR_GREEN                (3066993)
#define DISCORD_COLOR_DARK_GREEN           (2067276)
#define DISCORD_COLOR_BLUE                 (3447003)
#define DISCORD_COLOR_DARK_BLUE            (2123412)
#define DISCORD_COLOR_PURPLE               (10181046)
#define DISCORD_COLOR_DARK_PURPLE          (7419530)
#define DISCORD_COLOR_LUMINOUS_VIVID_PINK  (15277667)
#define DISCORD_COLOR_DARK_VIVID_PINK      (11342935)
#define DISCORD_COLOR_GOLD                 (15844367)
#define DISCORD_COLOR_DARK_GOLD            (12745742)
#define DISCORD_COLOR_ORANGE               (15105570)
#define DISCORD_COLOR_DARK_ORANGE          (11027200)
#define DISCORD_COLOR_RED                  (15158332)
#define DISCORD_COLOR_DARK_RED             (10038562)
#define DISCORD_COLOR_GREY                 (9807270)
#define DISCORD_COLOR_DARK_GREY            (9936031)
#define DISCORD_COLOR_DARKER_GREY          (8359053)
#define DISCORD_COLOR_LIGHT_GREY           (12370112)
#define DISCORD_COLOR_NAVY                 (3426654)
#define DISCORD_COLOR_DARK_NAVY            (2899536)
#define DISCORD_COLOR_YELLOW               (16776960)
#define DISCORD_COLOR_WHITE                (16777215)
#define DISCORD_COLOR_BLURPLE              (5793266)
#define DISCORD_COLOR_GREYPLE              (10070709)
#define DISCORD_COLOR_DARK_BUT_NOT_BLACK   (2895667)
#define DISCORD_COLOR_NOT_QUITE_BLACK      (2303786)
#define DISCORD_COLOR_FUSCHIA              (15418782)

typedef struct discord* discord_handle_t;

typedef struct {
    char* token;
    int intents;
    size_t gateway_buffer_size;
    size_t api_buffer_size;
    size_t api_timeout_ms;
    uint8_t queue_size;
    size_t task_stack_size;
    uint8_t task_priority;
} discord_config_t;

typedef enum {
    DISCORD_STATE_ERROR = -2,
    DISCORD_STATE_DISCONNECTED = -1,   /*<! Disconnected from gateway */
    DISCORD_STATE_UNKNOWN,             /*<! Not initialized */
    DISCORD_STATE_INIT,                /*<! Initialized but not open */
    DISCORD_STATE_OPEN,                /*<! Open and waiting to connect with gateway WebSocket server */
    DISCORD_STATE_CONNECTING,          /*<! Connected with gateway WebSocket server and waiting to identify... */
    DISCORD_STATE_CONNECTED,           /*<! Fully connected and identified with gateway */
    DISCORD_STATE_DISCONNECTING        /*<! In process of disconnection from gateway */
} discord_gateway_state_t;

typedef enum {
    DISCORD_CLOSEOP_NO_CODE,
    DISCORD_CLOSEOP_UNKNOWN_ERROR = 4000,   /*!< We're not sure what went wrong. Try reconnecting? */
    DISCORD_CLOSEOP_UNKNOWN_OPCODE,         /*!< You sent an invalid Gateway opcode or an invalid payload for an opcode. Don't do that! */
    DISCORD_CLOSEOP_DECODE_ERROR,           /*!< You sent an invalid payload to us. Don't do that! */
    DISCORD_CLOSEOP_NOT_AUTHENTICATED,      /*!< You sent us a payload prior to identifying. */
    DISCORD_CLOSEOP_AUTHENTICATION_FAILED,  /*!< The account token sent with your identify payload is incorrect. */
    DISCORD_CLOSEOP_ALREADY_AUTHENTICATED,  /*!< You sent more than one identify payload. Don't do that! */
    DISCORD_CLOSEOP_INVALID_SEQ = 4007,     /*!< The sequence sent when resuming the session was invalid. Reconnect and start a new session. */
    DISCORD_CLOSEOP_RATE_LIMITED,           /*!< Woah nelly! You're sending payloads to us too quickly. Slow it down! You will be disconnected on receiving this. */
    DISCORD_CLOSEOP_SESSION_TIMED_OUT,      /*!< Your session timed out. Reconnect and start a new one. */
    DISCORD_CLOSEOP_INVALID_SHARD,          /*!< You sent us an invalid shard when identifying. */
    DISCORD_CLOSEOP_SHARDING_REQUIRED,      /*!< The session would have handled too many guilds - you are required to shard your connection in order to connect. */
    DISCORD_CLOSEOP_INVALID_API_VERSION,    /*!< You sent an invalid version for the gateway. */
    DISCORD_CLOSEOP_INVALID_INTENTS,        /*!< You sent an invalid intent for a Gateway Intent. You may have incorrectly calculated the bitwise value. */
    DISCORD_CLOSEOP_DISALLOWED_INTENTS      /*!< You sent a disallowed intent for a Gateway Intent. You may have tried to specify an intent that you have not enabled or are not whitelisted for. */
} discord_close_code_t;

ESP_EVENT_DECLARE_BASE(DISCORD_EVENTS);

typedef enum {
    DISCORD_EVENT_ANY = ESP_EVENT_ANY_ID,
    DISCORD_EVENT_NONE,
    DISCORD_EVENT_UNKNOWN,                     /*<! Unknown event. This will probably never be fired */
    DISCORD_EVENT_DISCONNECTED,                /*<! Bot is fully disconnected from discord. It's not going to automatically reconnect. To connect again, discord_login function needs to be called */
    DISCORD_EVENT_RECONNECTING,                /*<! Something happened and bot is going to try to reconnect */
    DISCORD_EVENT_READY,                       /*<! This event will never be fired. Use DISCORD_EVENT_CONNECTED instead */
    DISCORD_EVENT_CONNECTED,                   /*<! Bot is fully connected and identified on discord. It's ready for action */
    DISCORD_EVENT_MESSAGE_RECEIVED,            /*<! Message received */
    DISCORD_EVENT_MESSAGE_UPDATED,             /*<! Message updated */
    DISCORD_EVENT_MESSAGE_DELETED,             /*<! Message deleted */
    DISCORD_EVENT_MESSAGE_REACTION_ADDED,      /*<! Reaction added to message */
    DISCORD_EVENT_MESSAGE_REACTION_REMOVED,    /*<! Reaction removed from message */
    DISCORD_EVENT_VOICE_STATE_UPDATED,         /*<! Voice state updated */
} discord_event_t;

typedef void* discord_event_data_ptr_t;

typedef struct {
    discord_handle_t client;
    discord_event_data_ptr_t ptr;
} discord_event_data_t;

typedef struct {
    const void* data;
    size_t length;
    size_t offset;
    size_t total_length;
} discord_download_info_t;

typedef esp_err_t(*discord_download_handler_t)(discord_download_info_t* info, void* arg);

discord_handle_t discord_create(const discord_config_t* config);
/**
 * @brief Cannot be called from event handler
 */
esp_err_t discord_login(discord_handle_t client);
esp_err_t discord_register_events(discord_handle_t client, discord_event_t event, esp_event_handler_t event_handler, void* event_handler_arg);
esp_err_t discord_unregister_events(discord_handle_t client, discord_event_t event, esp_event_handler_t event_handler);
esp_err_t discord_get_state(discord_handle_t client, discord_gateway_state_t* out_state);
esp_err_t discord_get_close_code(discord_handle_t client, discord_close_code_t* out_code);
/**
 * @brief Cannot be called from event handler
 */
esp_err_t discord_logout(discord_handle_t client);
/**
 * @brief Cannot be called from event handler
 */
esp_err_t discord_destroy(discord_handle_t client);

/**
 * @brief Get time in miliseconds since boot
 * @return number of miliseconds since esp_timer_init was called (this normally happens early during application startup)
 */
uint64_t discord_tick_ms();

#ifdef __cplusplus
}
#endif

#endif