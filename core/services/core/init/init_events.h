#ifndef BHARAT_INIT_EVENTS_H
#define BHARAT_INIT_EVENTS_H

#include <stdint.h>
#include <stdbool.h>
#include "init_manifest.h"
#include "init_status.h"

typedef enum {
    INIT_EVENT_NONE = 0,
    INIT_EVENT_SERVICE_REGISTERED,
    INIT_EVENT_SERVICE_READY,
    INIT_EVENT_SERVICE_FAILED,
    INIT_EVENT_TIMEOUT,
    INIT_EVENT_HANDOFF_ACK,
} init_event_type_t;

typedef struct {
    init_event_type_t type;
    init_service_id_t service_id;
    int status_code;
    uint32_t deadline_id;
} init_event_t;

#define INIT_EVENT_QUEUE_CAPACITY 32

typedef struct {
    init_event_t entries[INIT_EVENT_QUEUE_CAPACITY];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
} init_event_queue_t;

void init_event_queue_init(init_event_queue_t *q);
bool init_event_push(init_event_queue_t *q, const init_event_t *ev);
bool init_event_pop(init_event_queue_t *q, init_event_t *ev);

#endif // BHARAT_INIT_EVENTS_H
