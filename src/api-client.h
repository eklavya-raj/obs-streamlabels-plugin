#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <stdbool.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct api_client api_client;

typedef struct {
    char *socket_token;
} socket_token_response_t;

typedef struct {
    char *key;
    char *value;
} string_pair_t;

typedef struct {
    string_pair_t *data;
    size_t count;
} label_data_response_t;

typedef struct {
    char *format;
    char *item_format;
    char *item_separator;
    int limit;
    int duration;
    char *show_clock;
    char *show_count;
    char *show_latest;
    bool include_resubs;
} label_settings_t;

typedef struct {
    label_settings_t *settings;
    size_t count;
} label_settings_response_t;

typedef struct {
    char *label;
    char *name;
    char *settings_stat;
    char **settings_whitelist;
    size_t whitelist_count;
} label_definition_file_t;

typedef struct {
    char *category_label;
    label_definition_file_t *files;
    size_t file_count;
} label_category_t;

typedef struct {
    label_category_t *categories;
    size_t category_count;
} label_definitions_response_t;

api_client *api_client_create(void);
void api_client_destroy(api_client *client);

bool api_client_set_token(api_client *client, const char *token);
const char *api_client_get_token(api_client *client);

bool api_client_fetch_socket_token(api_client *client, socket_token_response_t *response);
bool api_client_fetch_initial_data(api_client *client, label_data_response_t *response);
bool api_client_fetch_settings(api_client *client, label_settings_response_t *response);
bool api_client_fetch_definitions(api_client *client, const char *platform, label_definitions_response_t *response);
bool api_client_update_settings(api_client *client, label_settings_response_t *settings);
bool api_client_restart_session(api_client *client);

void socket_token_response_free(socket_token_response_t *response);
void label_data_response_free(label_data_response_t *response);
void label_settings_response_free(label_settings_response_t *response);
void label_definitions_response_free(label_definitions_response_t *response);

#ifdef __cplusplus
}
#endif

#endif
