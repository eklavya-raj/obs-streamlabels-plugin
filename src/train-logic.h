#ifndef TRAIN_LOGIC_H
#define TRAIN_LOGIC_H

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TRAIN_DONATION,
    TRAIN_FOLLOW,
    TRAIN_SUBSCRIPTION,
    TRAIN_BITS,
    TRAIN_STARS,
    TRAIN_SUPERCHAT,
    TRAIN_SPONSOR,
    TRAIN_SUPPORT,
    TRAIN_YOUTUBE_SUBSCRIBER,
    TRAIN_FACEBOOK_FOLLOW,
    TRAIN_TROVO_FOLLOW,
    TRAIN_TROVO_SUBSCRIPTION,
    TRAIN_COUNT
} train_type_t;

typedef struct {
    int64_t most_recent_event_at;
    char *most_recent_name;
    int counter;
    double most_recent_amount;
    double total_amount;
    int duration_seconds;
    bool donation_train;
} train_info_t;

typedef struct {
    train_info_t trains[TRAIN_COUNT];
    pthread_mutex_t mutex;
    bool running;
    pthread_t timer_thread;
} train_logic_t;

train_logic_t *train_logic_create(void);
void train_logic_destroy(train_logic_t *logic);

void train_logic_start(train_logic_t *logic);
void train_logic_stop(train_logic_t *logic);

void train_logic_add_event(train_logic_t *logic, train_type_t type, const char *name, double amount);
void train_logic_set_duration(train_logic_t *logic, train_type_t type, int duration);

train_info_t *train_logic_get_train(train_logic_t *logic, train_type_t type);
const char *train_logic_get_clock(train_logic_t *logic, train_type_t type);
const char *train_logic_get_counter(train_logic_t *logic, train_type_t type);
const char *train_logic_get_latest_name(train_logic_t *logic, train_type_t type);
const char *train_logic_get_latest_amount(train_logic_t *logic, train_type_t type);
const char *train_logic_get_total_amount(train_logic_t *logic, train_type_t type);

#ifdef __cplusplus
}
#endif

#endif
