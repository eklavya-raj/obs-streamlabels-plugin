#include "settings.h"
#include <obs.h>
#include <util/config-file.h>
#include <string.h>
#include <stdlib.h>

#define SETTINGS_MODULE "obs-streamlabels"
#define SETTINGS_AUTH_TOKEN "auth_token"
#define SETTINGS_PLATFORM "platform"
#define SETTINGS_AUTO_CONNECT "auto_connect"

static char *auth_token = nullptr;
static char *platform = nullptr;
static bool auto_connect = true;

void settings_load(void)
{
    config_t *config = obs_frontend_get_global_config();
    
    if (config) {
        const char *token = config_get_string(config, SETTINGS_MODULE, SETTINGS_AUTH_TOKEN);
        if (token && *token) {
            auth_token = bstrdup(token);
        }
        
        const char *plat = config_get_string(config, SETTINGS_MODULE, SETTINGS_PLATFORM);
        if (plat && *plat) {
            platform = bstrdup(plat);
        } else {
            platform = bstrdup("twitch");
        }
        
        auto_connect = config_get_bool(config, SETTINGS_MODULE, SETTINGS_AUTO_CONNECT, true);
    }
    
    blog(LOG_INFO, "[Streamlabels] Settings loaded");
}

void settings_save(void)
{
    config_t *config = obs_frontend_get_global_config();
    
    if (config) {
        if (auth_token) {
            config_set_string(config, SETTINGS_MODULE, SETTINGS_AUTH_TOKEN, auth_token);
        }
        
        if (platform) {
            config_set_string(config, SETTINGS_MODULE, SETTINGS_PLATFORM, platform);
        }
        
        config_set_bool(config, SETTINGS_MODULE, SETTINGS_AUTO_CONNECT, auto_connect);
        
        config_save(config);
    }
    
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
