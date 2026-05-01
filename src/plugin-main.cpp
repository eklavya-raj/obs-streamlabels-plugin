#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/config-file.h>
#include <util/platform.h>
#include <util/threading.h>
#include "api-client.h"
#include "websocket-client.h"
#include "streamlabel-source.h"
#include "settings.h"
#include "train-logic.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-streamlabels", "en-US")

struct streamlabels_data {
    api_client *api;
    websocket_client *ws;
    pthread_mutex_t mutex;
    bool initialized;
};

static struct streamlabels_data *plugin_data = nullptr;

bool obs_module_load(void)
{
    blog(LOG_INFO, "[Streamlabels] Loading plugin");

    plugin_data = (struct streamlabels_data *)bzalloc(sizeof(struct streamlabels_data));
    if (!plugin_data) {
        blog(LOG_ERROR, "[Streamlabels] Failed to allocate plugin data");
        return false;
    }

    pthread_mutex_init(&plugin_data->mutex, nullptr);
    plugin_data->initialized = false;

    // Load settings
    settings_load();

    // Initialize API client
    plugin_data->api = api_client_create();
    if (!plugin_data->api) {
        blog(LOG_ERROR, "[Streamlabels] Failed to create API client");
        bfree(plugin_data);
        return false;
    }

    // Initialize WebSocket client
    plugin_data->ws = websocket_client_create(plugin_data->api);
    if (!plugin_data->ws) {
        blog(LOG_ERROR, "[Streamlabels] Failed to create WebSocket client");
        api_client_destroy(plugin_data->api);
        bfree(plugin_data);
        return false;
    }

    // Register stream label source
    streamlabel_source_register();

    plugin_data->initialized = true;

    blog(LOG_INFO, "[Streamlabels] Plugin loaded successfully");
    return true;
}

void obs_module_unload(void)
{
    blog(LOG_INFO, "[Streamlabels] Unloading plugin");

    if (plugin_data) {
        pthread_mutex_lock(&plugin_data->mutex);

        if (plugin_data->ws) {
            websocket_client_destroy(plugin_data->ws);
            plugin_data->ws = nullptr;
        }

        if (plugin_data->api) {
            api_client_destroy(plugin_data->api);
            plugin_data->api = nullptr;
        }

        pthread_mutex_unlock(&plugin_data->mutex);
        pthread_mutex_destroy(&plugin_data->mutex);

        bfree(plugin_data);
        plugin_data = nullptr;
    }

    settings_save();

    blog(LOG_INFO, "[Streamlabels] Plugin unloaded");
}

const char *obs_module_name(void)
{
    return "Streamlabels";
}

const char *obs_module_description(void)
{
    return "Streamlabs Stream Labels Plugin for OBS Studio";
}
