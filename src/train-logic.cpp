#include "train-logic.h"
#include <obs.h>
#include <util/platform.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define DEFAULT_TRAIN_DURATION 300 // 5 minutes

static void *timer_thread(void *data)
{
    train_logic_t *logic = (train_logic_t *)data;
    
    while (logic->running) {
        pthread_mutex_lock(&logic->mutex);
        
        int64_t now = os_gettime_ns() / 1000000; // Convert to milliseconds
        
        for (int i = 0; i < TRAIN_COUNT; i++) {
            train_info_t *train = &logic->trains[i];
            
            if (train->most_recent_event_at > 0) {
                int64_t elapsed = now - train->most_recent_event_at;
                int duration_ms = train->duration_seconds * 1000;
                
                if (elapsed >= duration_ms) {
                    // Clear the train
                    train->most_recent_event_at = 0;
                    train->counter = 0;
                    if (train->most_recent_name) {
                        bfree(train->most_recent_name);
                        train->most_recent_name = nullptr;
                    }
                    if (train->donation_train) {
                        train->most_recent_amount = 0;
                        train->total_amount = 0;
                    }
                }
            }
        }
        
        pthread_mutex_unlock(&logic->mutex);
        os_sleep_ms(1000);
    }
    
    return nullptr;
}

train_logic_t *train_logic_create(void)
{
    train_logic_t *logic = (train_logic_t *)bzalloc(sizeof(train_logic_t));
    if (!logic) return nullptr;
    
    pthread_mutex_init(&logic->mutex, nullptr);
    logic->running = false;
    
    // Initialize all trains with default duration
    for (int i = 0; i < TRAIN_COUNT; i++) {
        logic->trains[i].most_recent_event_at = 0;
        logic->trains[i].most_recent_name = nullptr;
        logic->trains[i].counter = 0;
        logic->trains[i].most_recent_amount = 0;
        logic->trains[i].total_amount = 0;
        logic->trains[i].duration_seconds = DEFAULT_TRAIN_DURATION;
        logic->trains[i].donation_train = (i == TRAIN_DONATION);
    }
    
    return logic;
}

void train_logic_destroy(train_logic_t *logic)
{
    if (!logic) return;
    
    train_logic_stop(logic);
    
    pthread_mutex_lock(&logic->mutex);
    
    for (int i = 0; i < TRAIN_COUNT; i++) {
        if (logic->trains[i].most_recent_name) {
            bfree(logic->trains[i].most_recent_name);
        }
    }
    
    pthread_mutex_unlock(&logic->mutex);
    pthread_mutex_destroy(&logic->mutex);
    
    bzfree(logic);
}

void train_logic_start(train_logic_t *logic)
{
    if (!logic || logic->running) return;
    
    logic->running = true;
    pthread_create(&logic->timer_thread, nullptr, timer_thread, logic);
}

void train_logic_stop(train_logic_t *logic)
{
    if (!logic || !logic->running) return;
    
    logic->running = false;
    pthread_join(logic->timer_thread, nullptr);
}

void train_logic_add_event(train_logic_t *logic, train_type_t type, const char *name, double amount)
{
    if (!logic || type < 0 || type >= TRAIN_COUNT) return;
    
    pthread_mutex_lock(&logic->mutex);
    
    train_info_t *train = &logic->trains[type];
    train->most_recent_event_at = os_gettime_ns() / 1000000;
    train->counter++;
    
    if (name) {
        if (train->most_recent_name) {
            bfree(train->most_recent_name);
        }
        train->most_recent_name = bstrdup(name);
    }
    
    if (train->donation_train && amount > 0) {
        train->most_recent_amount = amount;
        train->total_amount += amount;
    }
    
    pthread_mutex_unlock(&logic->mutex);
}

void train_logic_set_duration(train_logic_t *logic, train_type_t type, int duration)
{
    if (!logic || type < 0 || type >= TRAIN_COUNT) return;
    
    pthread_mutex_lock(&logic->mutex);
    logic->trains[type].duration_seconds = duration;
    pthread_mutex_unlock(&logic->mutex);
}

train_info_t *train_logic_get_train(train_logic_t *logic, train_type_t type)
{
    if (!logic || type < 0 || type >= TRAIN_COUNT) return nullptr;
    
    pthread_mutex_lock(&logic->mutex);
    train_info_t *train = &logic->trains[type];
    pthread_mutex_unlock(&logic->mutex);
    
    return train;
}

const char *train_logic_get_clock(train_logic_t *logic, train_type_t type)
{
    if (!logic || type < 0 || type >= TRAIN_COUNT) return nullptr;
    
    static char clock_str[16];
    
    pthread_mutex_lock(&logic->mutex);
    
    train_info_t *train = &logic->trains[type];
    
    if (train->most_recent_event_at == 0) {
        snprintf(clock_str, sizeof(clock_str), "0:00");
    } else {
        int64_t now = os_gettime_ns() / 1000000;
        int64_t elapsed = now - train->most_recent_event_at;
        int duration_ms = train->duration_seconds * 1000;
        int remaining_ms = duration_ms - elapsed;
        
        if (remaining_ms < 0) {
            remaining_ms = 0;
        }
        
        int minutes = remaining_ms / 60000;
        int seconds = (remaining_ms % 60000) / 1000;
        snprintf(clock_str, sizeof(clock_str), "%d:%02d", minutes, seconds);
    }
    
    pthread_mutex_unlock(&logic->mutex);
    
    return clock_str;
}

const char *train_logic_get_counter(train_logic_t *logic, train_type_t type)
{
    if (!logic || type < 0 || type >= TRAIN_COUNT) return nullptr;
    
    static char counter_str[32];
    
    pthread_mutex_lock(&logic->mutex);
    snprintf(counter_str, sizeof(counter_str), "%d", logic->trains[type].counter);
    pthread_mutex_unlock(&logic->mutex);
    
    return counter_str;
}

const char *train_logic_get_latest_name(train_logic_t *logic, train_type_t type)
{
    if (!logic || type < 0 || type >= TRAIN_COUNT) return nullptr;
    
    pthread_mutex_lock(&logic->mutex);
    const char *name = logic->trains[type].most_recent_name;
    pthread_mutex_unlock(&logic->mutex);
    
    return name;
}

const char *train_logic_get_latest_amount(train_logic_t *logic, train_type_t type)
{
    if (!logic || type < 0 || type >= TRAIN_COUNT) return nullptr;
    
    static char amount_str[32];
    
    pthread_mutex_lock(&logic->mutex);
    snprintf(amount_str, sizeof(amount_str), "%.2f", logic->trains[type].most_recent_amount);
    pthread_mutex_unlock(&logic->mutex);
    
    return amount_str;
}

const char *train_logic_get_total_amount(train_logic_t *logic, train_type_t type)
{
    if (!logic || type < 0 || type >= TRAIN_COUNT) return nullptr;
    
    static char amount_str[32];
    
    pthread_mutex_lock(&logic->mutex);
    snprintf(amount_str, sizeof(amount_str), "%.2f", logic->trains[type].total_amount);
    pthread_mutex_unlock(&logic->mutex);
    
    return amount_str;
}
