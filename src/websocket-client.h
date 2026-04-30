#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <stdbool.h>
#include <pthread.h>
#include "api-client.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct websocket_client websocket_client;

typedef enum {
    EVENT_STREAMLABELS,
    EVENT_DONATION,
    EVENT_FOLLOW,
    EVENT_SUBSCRIPTION,
    EVENT_BITS,
    EVENT_STARS,
    EVENT_SUPERCHAT,
    EVENT_SUPPORT,
    EVENT_UNKNOWN
} event_type_t;

typedef struct {
    event_type_t type;
    char *platform;
    char **data;
    size_t data_count;
} socket_event_t;

typedef void (*event_callback_t)(socket_event_t *event, void *user_data);

websocket_client *websocket_client_create(api_client *api);
void websocket_client_destroy(websocket_client *client);

bool websocket_client_connect(websocket_client *client);
void websocket_client_disconnect(websocket_client *client);
bool websocket_client_is_connected(websocket_client *client);

void websocket_client_set_event_callback(websocket_client *client, event_callback_t callback, void *user_data);

void socket_event_free(socket_event_t *event);

#ifdef __cplusplus
}
#endif

#endif
