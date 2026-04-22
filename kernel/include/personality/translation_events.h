#ifndef BHARAT_PERSONALITY_TRANSLATION_EVENTS_H
#define BHARAT_PERSONALITY_TRANSLATION_EVENTS_H

#include <stdint.h>

typedef enum {
    BH_TRANSLATION_EVENT_BOUNDARY_ENTER = 0,
    BH_TRANSLATION_EVENT_BOUNDARY_EXIT = 1,
    BH_TRANSLATION_EVENT_CACHE_MISS = 2,
    BH_TRANSLATION_EVENT_FALLBACK = 3,
    BH_TRANSLATION_EVENT_EXTRA_COPY = 4,
} bh_translation_event_t;

typedef struct {
    uint64_t boundary_enter;
    uint64_t boundary_exit;
    uint64_t cache_miss;
    uint64_t fallback;
    uint64_t extra_copy_events;
    uint64_t extra_copy_bytes;
} bh_translation_counters_t;

void bh_translation_event_record(bh_translation_event_t event);
void bh_translation_record_extra_copy(uint64_t bytes_copied);
void bh_translation_counters_snapshot(bh_translation_counters_t* out);
void bh_translation_counters_reset(void);

#endif // BHARAT_PERSONALITY_TRANSLATION_EVENTS_H
