#ifndef _DISCORD_MODELS_MEMBER_H_
#define _DISCORD_MODELS_MEMBER_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char* nick;
    char* permissions;
} discord_member_t;

void discord_member_free(discord_member_t* member);

#ifdef __cplusplus
}
#endif

#endif