#include "init_events.h"

void init_event_queue_init(init_event_queue_t *q) {
    if (!q) return;
    q->head = q->tail = q->count = 0;
}

bool init_event_push(init_event_queue_t *q, const init_event_t *ev) {
    if (!q || !ev || q->count >= INIT_EVENT_QUEUE_CAPACITY) {
        return false;
    }
    q->entries[q->tail] = *ev;
    q->tail = (q->tail + 1U) % INIT_EVENT_QUEUE_CAPACITY;
    q->count++;
    return true;
}

bool init_event_pop(init_event_queue_t *q, init_event_t *ev) {
    if (!q || !ev || q->count == 0) {
        return false;
    }
    *ev = q->entries[q->head];
    q->head = (q->head + 1U) % INIT_EVENT_QUEUE_CAPACITY;
    q->count--;
    return true;
}
