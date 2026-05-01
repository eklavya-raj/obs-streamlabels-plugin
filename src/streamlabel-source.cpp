#include "streamlabel-source.h"
#include "api-client.h"
#include <obs.h>
#include <util/dstr.h>
#include <string.h>
#include <stdlib.h>

#define STREAMLABEL_SOURCE_ID "streamlabel_source"
#define STREAMLABEL_SOURCE_NAME "Stream Label"

struct streamlabel_source {
    obs_source_t *source;
    char *statname;
    char *current_text;
    pthread_mutex_t mutex;
};

static const char *streamlabel_source_get_name(void *type_data)
{
    return STREAMLABEL_SOURCE_NAME;
}

static void streamlabel_source_update(void *data, obs_data_t *settings);

static void *streamlabel_source_create(obs_data_t *settings, obs_source_t *source)
{
    streamlabel_source *context = (streamlabel_source *)bzalloc(sizeof(streamlabel_source));
    if (!context) return nullptr;
    
    context->source = source;
    context->statname = nullptr;
    context->current_text = nullptr;
    pthread_mutex_init(&context->mutex, nullptr);
    
    streamlabel_source_update(context, settings);
    
    return context;
}

static void streamlabel_source_destroy(void *data)
{
    streamlabel_source *context = (streamlabel_source *)data;
    if (!context) return;
    
    pthread_mutex_destroy(&context->mutex);
    
    if (context->statname) {
        bfree(context->statname);
    }
    
    if (context->current_text) {
        bfree(context->current_text);
    }
    
    bfree(context);
}

static void streamlabel_source_update(void *data, obs_data_t *settings)
{
    streamlabel_source *context = (streamlabel_source *)data;
    if (!context) return;
    
    pthread_mutex_lock(&context->mutex);
    
    const char *statname = obs_data_get_string(settings, "statname");
    if (statname && *statname) {
        if (context->statname) {
            bfree(context->statname);
        }
        context->statname = bstrdup(statname);
    }
    
    pthread_mutex_unlock(&context->mutex);
}

static obs_properties_t *streamlabel_source_properties(void *data)
{
    obs_properties_t *props = obs_properties_create();
    
    obs_property_t *statname_prop = obs_properties_add_list(
        props,
        "statname",
        "Label Type",
        OBS_COMBO_TYPE_LIST,
        OBS_COMBO_FORMAT_STRING
    );
    
    // Add common stream label types
    obs_property_list_add_string(statname_prop, "Recent Follower", "recent_follower");
    obs_property_list_add_string(statname_prop, "Recent Donation", "recent_donation");
    obs_property_list_add_string(statname_prop, "Recent Subscriber", "recent_subscriber");
    obs_property_list_add_string(statname_prop, "Top Donator", "all_time_top_donator");
    obs_property_list_add_string(statname_prop, "Follower Goal", "follower_goal");
    obs_property_list_add_string(statname_prop, "Subscriber Goal", "subscriber_goal");
    obs_property_list_add_string(statname_prop, "Donation Goal", "donation_goal");
    obs_property_list_add_string(statname_prop, "Bit Goal", "bit_goal");
    obs_property_list_add_string(statname_prop, "Viewer Count", "current_viewer_count");
    obs_property_list_add_string(statname_prop, "Stream Title", "stream_title");
    obs_property_list_add_string(statname_prop, "Stream Game", "stream_game");
    
    return props;
}

static void streamlabel_source_get_defaults(obs_data_t *settings)
{
    obs_data_set_default_string(settings, "statname", "recent_follower");
}

static void streamlabel_source_render(void *data, gs_effect_t *effect)
{
    streamlabel_source *context = (streamlabel_source *)data;
    if (!context) return;
    
    // This is a text source, so we delegate to OBS text rendering
    // In a full implementation, you'd create a text source internally
    // and render it here
}

void streamlabel_source_register(void)
{
    struct obs_source_info info = {0};
    
    info.id = STREAMLABEL_SOURCE_ID;
    info.type = OBS_SOURCE_TYPE_INPUT;
    info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW;
    
    info.get_name = streamlabel_source_get_name;
    info.create = streamlabel_source_create;
    info.destroy = streamlabel_source_destroy;
    info.update = streamlabel_source_update;
    info.get_properties = streamlabel_source_properties;
    info.get_defaults = streamlabel_source_get_defaults;
    info.video_render = streamlabel_source_render;
    
    obs_register_source(&info);
    
    blog(LOG_INFO, "[Streamlabels] Registered stream label source");
}
