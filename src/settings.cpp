#include "settings.h"
#include <obs.h>
#include <string.h>
#include <stdlib.h>

#define SETTINGS_MODULE "obs-streamlabels"

static char *auth_token = nullptr;
static char *platform = nullptr;
static bool auto_connect = true;

void settings_load(void)
{
    // Simplified: settings are loaded from static variables
    // In a full implementation, use obs_module_get_config_path to load from file
    if (!platform) {
        platform = bstrdup("twitch");
    }
    
    blog(LOG_INFO, "[Streamlabels] Settings loaded");
}

void settings_save(void)
{
    // Simplified: settings are kept in memory
    // In a full implementation, use obs_module_get_config_path to save to file
    blog(LOG_INFO, "[Streamlabels] Settings saved");
}

void settings_free(void)
{
    if (auth_token) {
        bfree(auth_token);
        auth_token = nullptr;
    }
    
    if (platform) {
        bfree(platform);
        platform = nullptr;
    }
}

const char *settings_get_auth_token(void)
{
    return auth_token;
}

void settings_set_auth_token(const char *token)
{
    if (auth_token) {
        bfree(auth_token);
    }
    
    auth_token = token ? bstrdup(token) : nullptr;
}

const char *settings_get_platform(void)
{
    return platform;
}

void settings_set_platform(const char *plat)
{
    if (platform) {
        bfree(platform);
    }
    
    platform = plat ? bstrdup(plat) : nullptr;
}

bool settings_get_auto_connect(void)
{
    return auto_connect;
}

void settings_set_auto_connect(bool connect)
{
    auto_connect = connect;
}
