#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void settings_load(void);
void settings_save(void);
void settings_free(void);

const char *settings_get_auth_token(void);
void settings_set_auth_token(const char *token);

const char *settings_get_platform(void);
void settings_set_platform(const char *platform);

bool settings_get_auto_connect(void);
void settings_set_auto_connect(bool auto_connect);

#ifdef __cplusplus
}
#endif

#endif
