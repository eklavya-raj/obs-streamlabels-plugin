#include "api-client.h"
#include <obs.h>
#include <curl/curl.h>
#include <cJSON.h>
#include <string.h>
#include <stdlib.h>

#define STREAMLABS_API_BASE "https://streamlabs.com/api/v5/slobs"
#define STREAMLABS_SOCKET_TOKEN_ENDPOINT "/socket-token"
#define STREAMLABS_FILES_ENDPOINT "/stream-labels/files"
#define STREAMLABS_SETTINGS_ENDPOINT "/stream-labels/settings"
#define STREAMLABS_DEFINITIONS_ENDPOINT "/stream-labels/app-settings"

struct api_client {
    char *token;
    pthread_mutex_t mutex;
};

typedef struct {
    char *data;
    size_t size;
} api_memory_blob;

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t total_size = size * nmemb;
    api_memory_blob *blob = (api_memory_blob *)userp;
    
    char *new_data = (char *)realloc(blob->data, blob->size + total_size + 1);
    if (!new_data) return 0;
    
    blob->data = new_data;
    memcpy((char *)blob->data + blob->size, contents, total_size);
    blob->size += total_size;
    ((char *)blob->data)[blob->size] = '\0';
    
    return total_size;
}

api_client *api_client_create(void)
{
    api_client *client = (api_client *)bzalloc(sizeof(api_client));
    if (!client) return nullptr;
    
    client->token = nullptr;
    pthread_mutex_init(&client->mutex, nullptr);
    
    return client;
}

void api_client_destroy(api_client *client)
{
    if (!client) return;
    
    pthread_mutex_destroy(&client->mutex);
    
    if (client->token) {
        bfree(client->token);
        client->token = nullptr;
    }
    
    bfree(client);
}

bool api_client_set_token(api_client *client, const char *token)
{
    if (!client || !token) return false;
    
    pthread_mutex_lock(&client->mutex);
    
    if (client->token) {
        bfree(client->token);
    }
    
    client->token = bstrdup(token);
    
    pthread_mutex_unlock(&client->mutex);
    return true;
}

const char *api_client_get_token(api_client *client)
{
    if (!client) return nullptr;
    
    pthread_mutex_lock(&client->mutex);
    const char *token = client->token;
    pthread_mutex_unlock(&client->mutex);
    
    return token;
}

static bool perform_request(api_client *client, const char *endpoint, const char *method, 
                           const char *post_data, char **response_data)
{
    if (!client || !endpoint || !response_data) return false;
    
    pthread_mutex_lock(&client->mutex);
    
    if (!client->token) {
        pthread_mutex_unlock(&client->mutex);
        blog(LOG_ERROR, "[API Client] No token set");
        return false;
    }
    
    char url[512];
    snprintf(url, sizeof(url), "%s%s", STREAMLABS_API_BASE, endpoint);
    
    api_memory_blob blob = {nullptr, 0};
    CURL *curl = curl_easy_init();
    if (!curl) {
        pthread_mutex_unlock(&client->mutex);
        return false;
    }
    
    struct curl_slist *headers = nullptr;
    char auth_header[256];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", client->token);
    headers = curl_slist_append(headers, auth_header);
    
    if (post_data) {
        headers = curl_slist_append(headers, "Content-Type: application/json");
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &blob);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    
    if (post_data) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
    }
    
    CURLcode res = curl_easy_perform(curl);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    pthread_mutex_unlock(&client->mutex);
    
    if (res != CURLE_OK) {
        blog(LOG_ERROR, "[API Client] Request failed: %s", curl_easy_strerror(res));
        if (blob.data) free(blob.data);
        return false;
    }
    
    *response_data = (char *)blob.data;
    return true;
}

bool api_client_fetch_socket_token(api_client *client, socket_token_response_t *response)
{
    if (!client || !response) return false;
    
    char *response_data = nullptr;
    if (!perform_request(client, STREAMLABS_SOCKET_TOKEN_ENDPOINT, "GET", nullptr, &response_data)) {
        return false;
    }
    
    cJSON *json = cJSON_Parse(response_data);
    free(response_data);
    
    if (!json) {
        blog(LOG_ERROR, "[API Client] Failed to parse socket token response");
        return false;
    }
    
    cJSON *token_obj = cJSON_GetObjectItem(json, "socket_token");
    if (!token_obj || !cJSON_IsString(token_obj)) {
        cJSON_Delete(json);
        return false;
    }
    
    response->socket_token = bstrdup(token_obj->valuestring);
    cJSON_Delete(json);
    
    return true;
}

bool api_client_fetch_initial_data(api_client *client, label_data_response_t *response)
{
    if (!client || !response) return false;
    
    char *response_data = nullptr;
    if (!perform_request(client, STREAMLABS_FILES_ENDPOINT, "GET", nullptr, &response_data)) {
        return false;
    }
    
    cJSON *json = cJSON_Parse(response_data);
    free(response_data);
    
    if (!json) {
        blog(LOG_ERROR, "[API Client] Failed to parse initial data response");
        return false;
    }
    
    cJSON *data_obj = cJSON_GetObjectItem(json, "data");
    if (!data_obj || !cJSON_IsObject(data_obj)) {
        cJSON_Delete(json);
        return false;
    }
    
    response->count = cJSON_GetArraySize(data_obj);
    response->data = (string_pair_t *)bzalloc(sizeof(string_pair_t) * response->count);
    
    cJSON *item = nullptr;
    size_t i = 0;
    cJSON_ArrayForEach(item, data_obj) {
        response->data[i].key = bstrdup(item->string);
        response->data[i].value = bstrdup(item->valuestring);
        i++;
    }
    
    cJSON_Delete(json);
    return true;
}

bool api_client_fetch_settings(api_client *client, label_settings_response_t *response)
{
    if (!client || !response) return false;
    
    char *response_data = nullptr;
    if (!perform_request(client, STREAMLABS_SETTINGS_ENDPOINT, "GET", nullptr, &response_data)) {
        return false;
    }
    
    cJSON *json = cJSON_Parse(response_data);
    free(response_data);
    
    if (!json) {
        blog(LOG_ERROR, "[API Client] Failed to parse settings response");
        return false;
    }
    
    response->count = cJSON_GetArraySize(json);
    response->settings = (label_settings_t *)bzalloc(sizeof(label_settings_t) * response->count);
    
    cJSON *item = nullptr;
    size_t i = 0;
    cJSON_ArrayForEach(item, json) {
        label_settings_t *setting = &response->settings[i];
        
        cJSON *format = cJSON_GetObjectItem(item, "format");
        if (format && cJSON_IsString(format)) {
            setting->format = bstrdup(format->valuestring);
        }
        
        cJSON *item_format = cJSON_GetObjectItem(item, "item_format");
        if (item_format && cJSON_IsString(item_format)) {
            setting->item_format = bstrdup(item_format->valuestring);
        }
        
        cJSON *item_separator = cJSON_GetObjectItem(item, "item_separator");
        if (item_separator && cJSON_IsString(item_separator)) {
            setting->item_separator = bstrdup(item_separator->valuestring);
        }
        
        cJSON *limit = cJSON_GetObjectItem(item, "limit");
        if (limit && cJSON_IsNumber(limit)) {
            setting->limit = limit->valueint;
        }
        
        cJSON *duration = cJSON_GetObjectItem(item, "duration");
        if (duration && cJSON_IsNumber(duration)) {
            setting->duration = duration->valueint;
        }
        
        cJSON *show_clock = cJSON_GetObjectItem(item, "show_clock");
        if (show_clock && cJSON_IsString(show_clock)) {
            setting->show_clock = bstrdup(show_clock->valuestring);
        }
        
        cJSON *show_count = cJSON_GetObjectItem(item, "show_count");
        if (show_count && cJSON_IsString(show_count)) {
            setting->show_count = bstrdup(show_count->valuestring);
        }
        
        cJSON *show_latest = cJSON_GetObjectItem(item, "show_latest");
        if (show_latest && cJSON_IsString(show_latest)) {
            setting->show_latest = bstrdup(show_latest->valuestring);
        }
        
        cJSON *include_resubs = cJSON_GetObjectItem(item, "include_resubs");
        if (include_resubs && cJSON_IsBool(include_resubs)) {
            setting->include_resubs = cJSON_IsTrue(include_resubs);
        }
        
        i++;
    }
    
    cJSON_Delete(json);
    return true;
}

bool api_client_fetch_definitions(api_client *client, const char *platform, label_definitions_response_t *response)
{
    if (!client || !platform || !response) return false;
    
    char endpoint[256];
    snprintf(endpoint, sizeof(endpoint), "%s/%s", STREAMLABS_DEFINITIONS_ENDPOINT, platform);
    
    char *response_data = nullptr;
    if (!perform_request(client, endpoint, "GET", nullptr, &response_data)) {
        return false;
    }
    
    cJSON *json = cJSON_Parse(response_data);
    free(response_data);
    
    if (!json) {
        blog(LOG_ERROR, "[API Client] Failed to parse definitions response");
        return false;
    }
    
    response->category_count = cJSON_GetArraySize(json);
    response->categories = (label_category_t *)bzalloc(sizeof(label_category_t) * response->category_count);
    
    cJSON *category = nullptr;
    size_t cat_i = 0;
    cJSON_ArrayForEach(category, json) {
        label_category_t *cat = &response->categories[cat_i];
        cat->category_label = bstrdup(category->string);
        
        cJSON *files_obj = cJSON_GetObjectItem(category, "files");
        if (files_obj && cJSON_IsArray(files_obj)) {
            cat->file_count = cJSON_GetArraySize(files_obj);
            cat->files = (label_definition_file_t *)bzalloc(sizeof(label_definition_file_t) * cat->file_count);
            
            cJSON *file = nullptr;
            size_t file_i = 0;
            cJSON_ArrayForEach(file, files_obj) {
                label_definition_file_t *file_def = &cat->files[file_i];
                
                cJSON *name = cJSON_GetObjectItem(file, "name");
                if (name && cJSON_IsString(name)) {
                    file_def->name = bstrdup(name->valuestring);
                }
                
                cJSON *label = cJSON_GetObjectItem(file, "label");
                if (label && cJSON_IsString(label)) {
                    file_def->label = bstrdup(label->valuestring);
                }
                
                cJSON *settings = cJSON_GetObjectItem(file, "settings");
                if (settings && cJSON_IsObject(settings)) {
                    cJSON *settings_stat = cJSON_GetObjectItem(settings, "settingsStat");
                    if (settings_stat && cJSON_IsString(settings_stat)) {
                        file_def->settings_stat = bstrdup(settings_stat->valuestring);
                    }
                    
                    cJSON *whitelist = cJSON_GetObjectItem(settings, "settingsWhitelist");
                    if (whitelist && cJSON_IsArray(whitelist)) {
                        file_def->whitelist_count = cJSON_GetArraySize(whitelist);
                        file_def->settings_whitelist = (char **)bzalloc(sizeof(char *) * file_def->whitelist_count);
                        
                        cJSON *whitelist_item = nullptr;
                        size_t whitelist_i = 0;
                        cJSON_ArrayForEach(whitelist_item, whitelist) {
                            if (cJSON_IsString(whitelist_item)) {
                                file_def->settings_whitelist[whitelist_i] = bstrdup(whitelist_item->valuestring);
                                whitelist_i++;
                            }
                        }
                    }
                }
                
                file_i++;
            }
        }
        
        cat_i++;
    }
    
    cJSON_Delete(json);
    return true;
}

bool api_client_update_settings(api_client *client, label_settings_response_t *settings)
{
    if (!client || !settings) return false;
    
    cJSON *json = cJSON_CreateObject();
    
    for (size_t i = 0; i < settings->count; i++) {
        label_settings_t *setting = &settings->settings[i];
        // TODO: Implement proper key-value mapping
        // This is a simplified version
    }
    
    char *json_str = cJSON_Print(json);
    bool result = perform_request(client, STREAMLABS_SETTINGS_ENDPOINT, "POST", json_str, nullptr);
    
    free(json_str);
    cJSON_Delete(json);
    
    return result;
}

bool api_client_restart_session(api_client *client)
{
    if (!client) return false;
    
    char endpoint[256];
    snprintf(endpoint, sizeof(endpoint), "%s/restart-session", STREAMLABS_SETTINGS_ENDPOINT);
    
    return perform_request(client, endpoint, "POST", nullptr, nullptr);
}

void socket_token_response_free(socket_token_response_t *response)
{
    if (!response) return;
    if (response->socket_token) bfree(response->socket_token);
    response->socket_token = nullptr;
}

void label_data_response_free(label_data_response_t *response)
{
    if (!response) return;
    
    for (size_t i = 0; i < response->count; i++) {
        if (response->data[i].key) bfree(response->data[i].key);
        if (response->data[i].value) bfree(response->data[i].value);
    }
    
    if (response->data) bfree(response->data);
    response->data = nullptr;
    response->count = 0;
}

void label_settings_response_free(label_settings_response_t *response)
{
    if (!response) return;
    
    for (size_t i = 0; i < response->count; i++) {
        label_settings_t *setting = &response->settings[i];
        if (setting->format) bfree(setting->format);
        if (setting->item_format) bfree(setting->item_format);
        if (setting->item_separator) bfree(setting->item_separator);
        if (setting->show_clock) bfree(setting->show_clock);
        if (setting->show_count) bfree(setting->show_count);
        if (setting->show_latest) bfree(setting->show_latest);
    }
    
    if (response->settings) bfree(response->settings);
    response->settings = nullptr;
    response->count = 0;
}

void label_definitions_response_free(label_definitions_response_t *response)
{
    if (!response) return;
    
    for (size_t i = 0; i < response->category_count; i++) {
        label_category_t *cat = &response->categories[i];
        if (cat->category_label) bfree(cat->category_label);
        
        for (size_t j = 0; j < cat->file_count; j++) {
            label_definition_file_t *file = &cat->files[j];
            if (file->name) bfree(file->name);
            if (file->label) bfree(file->label);
            if (file->settings_stat) bfree(file->settings_stat);
            
            for (size_t k = 0; k < file->whitelist_count; k++) {
                if (file->settings_whitelist[k]) bfree(file->settings_whitelist[k]);
            }
            if (file->settings_whitelist) bfree(file->settings_whitelist);
        }
        
        if (cat->files) bfree(cat->files);
    }
    
    if (response->categories) bfree(response->categories);
    response->categories = nullptr;
    response->category_count = 0;
}
