#include "websocket-client.h"
#include <obs.h>
#include <libwebsockets.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <thread>
#include <chrono>

#define STREAMLABS_WS_URL "aws-io.streamlabs.com"
#define STREAMLABS_WS_PATH "/"

struct websocket_client {
    api_client *api;
    struct lws_context *context;
    struct lws *wsi;
    pthread_t thread;
    pthread_mutex_t mutex;
    bool connected;
    bool should_stop;
    char *socket_token;
    event_callback_t event_callback;
    void *callback_user_data;
};

static int websocket_callback(struct lws *wsi, enum lws_callback_reasons reason,
                              void *user, void *in, size_t len)
{
    websocket_client *client = (websocket_client *)user;
    
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            blog(LOG_INFO, "[WebSocket] Connected");
            client->connected = true;
            break;
            
        case LWS_CALLBACK_CLIENT_RECEIVE:
            if (client && client->event_callback) {
                // Parse the received message
                // For now, this is a simplified version
                // In production, you'd parse JSON and create proper socket_event_t
                socket_event_t event;
                event.type = EVENT_STREAMLABELS;
                event.platform = nullptr;
                event.data = nullptr;
                event.data_count = 0;
                client->event_callback(&event, client->callback_user_data);
            }
            break;
            
        case LWS_CALLBACK_CLIENT_CLOSED:
            blog(LOG_INFO, "[WebSocket] Disconnected");
            client->connected = false;
            break;
            
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            blog(LOG_ERROR, "[WebSocket] Connection error");
            client->connected = false;
            break;
            
        default:
            break;
    }
    
    return 0;
}

static void *websocket_thread(void *data)
{
    websocket_client *client = (websocket_client *)data;
    
    while (!client->should_stop) {
        pthread_mutex_lock(&client->mutex);
        
        if (client->context) {
            lws_service(client->context, 0);
        }
        
        pthread_mutex_unlock(&client->mutex);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return nullptr;
}

websocket_client *websocket_client_create(api_client *api)
{
    if (!api) return nullptr;
    
    websocket_client *client = (websocket_client *)bzalloc(sizeof(websocket_client));
    if (!client) return nullptr;
    
    client->api = api;
    client->context = nullptr;
    client->wsi = nullptr;
    client->connected = false;
    client->should_stop = false;
    client->socket_token = nullptr;
    client->event_callback = nullptr;
    client->callback_user_data = nullptr;
    
    pthread_mutex_init(&client->mutex, nullptr);
    
    return client;
}

void websocket_client_destroy(websocket_client *client)
{
    if (!client) return;
    
    websocket_client_disconnect(client);
    
    if (client->thread) {
        client->should_stop = true;
        pthread_join(client->thread, nullptr);
    }
    
    pthread_mutex_destroy(&client->mutex);
    
    if (client->socket_token) {
        bfree(client->socket_token);
    }
    
    bfree(client);
}

bool websocket_client_connect(websocket_client *client)
{
    if (!client) return false;
    
    // Fetch socket token
    socket_token_response_t token_response;
    if (!api_client_fetch_socket_token(client->api, &token_response)) {
        blog(LOG_ERROR, "[WebSocket] Failed to fetch socket token");
        return false;
    }
    
    client->socket_token = token_response.socket_token;
    
    pthread_mutex_lock(&client->mutex);
    
    struct lws_context_creation_info info = {0};
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = nullptr;
    info.ssl_cert_filepath = nullptr;
    info.ssl_private_key_filepath = nullptr;
    info.gid = -1;
    info.uid = -1;
    
    client->context = lws_create_context(&info);
    if (!client->context) {
        blog(LOG_ERROR, "[WebSocket] Failed to create context");
        pthread_mutex_unlock(&client->mutex);
        return false;
    }
    
    // Create connection
    struct lws_client_connect_info connect_info = {0};
    connect_info.context = client->context;
    connect_info.address = STREAMLABS_WS_URL;
    connect_info.port = 443;
    connect_info.path = STREAMLABS_WS_PATH;
    connect_info.ssl_connection = LCCSCF_USE_SSL;
    connect_info.host = STREAMLABS_WS_URL;
    connect_info.origin = STREAMLABS_WS_URL;
    connect_info.protocol = nullptr;
    
    char url_with_token[512];
    snprintf(url_with_token, sizeof(url_with_token), "?token=%s", client->socket_token);
    connect_info.path = url_with_token;
    
    client->wsi = lws_client_connect_via_info(&connect_info);
    if (!client->wsi) {
        blog(LOG_ERROR, "[WebSocket] Failed to connect");
        lws_context_destroy(client->context);
        client->context = nullptr;
        pthread_mutex_unlock(&client->mutex);
        return false;
    }
    
    // Start service thread
    if (pthread_create(&client->thread, nullptr, websocket_thread, client) != 0) {
        blog(LOG_ERROR, "[WebSocket] Failed to create thread");
        lws_context_destroy(client->context);
        client->context = nullptr;
        pthread_mutex_unlock(&client->mutex);
        return false;
    }
    
    pthread_mutex_unlock(&client->mutex);
    
    blog(LOG_INFO, "[WebSocket] Connection initiated");
    return true;
}

void websocket_client_disconnect(websocket_client *client)
{
    if (!client) return;
    
    pthread_mutex_lock(&client->mutex);
    
    if (client->wsi) {
        lws_close_reason(client->wsi, LWS_CLOSE_STATUS_NORMAL, nullptr, 0);
        client->wsi = nullptr;
    }
    
    if (client->context) {
        lws_context_destroy(client->context);
        client->context = nullptr;
    }
    
    client->connected = false;
    
    pthread_mutex_unlock(&client->mutex);
}

bool websocket_client_is_connected(websocket_client *client)
{
    if (!client) return false;
    
    pthread_mutex_lock(&client->mutex);
    bool connected = client->connected;
    pthread_mutex_unlock(&client->mutex);
    
    return connected;
}

void websocket_client_set_event_callback(websocket_client *client, event_callback_t callback, void *user_data)
{
    if (!client) return;
    
    pthread_mutex_lock(&client->mutex);
    client->event_callback = callback;
    client->callback_user_data = user_data;
    pthread_mutex_unlock(&client->mutex);
}

void socket_event_free(socket_event_t *event)
{
    if (!event) return;
    
    if (event->platform) {
        bfree(event->platform);
    }
    
    if (event->data) {
        for (size_t i = 0; i < event->data_count; i++) {
            if (event->data[i]) {
                bfree(event->data[i]);
            }
        }
        bfree(event->data);
    }
    
    event->type = EVENT_UNKNOWN;
    event->platform = nullptr;
    event->data = nullptr;
    event->data_count = 0;
}
