#include "personality/translation_events.h"

#include <stddef.h>

static bh_translation_counters_t g_translation_counters;

void bh_translation_event_record(bh_translation_event_t event) {
    switch (event) {
        case BH_TRANSLATION_EVENT_BOUNDARY_ENTER:
            g_translation_counters.boundary_enter++;
            break;
        case BH_TRANSLATION_EVENT_BOUNDARY_EXIT:
            g_translation_counters.boundary_exit++;
            break;
        case BH_TRANSLATION_EVENT_CACHE_MISS:
            g_translation_counters.cache_miss++;
            break;
        case BH_TRANSLATION_EVENT_FALLBACK:
            g_translation_counters.fallback++;
            break;
        case BH_TRANSLATION_EVENT_EXTRA_COPY:
            g_translation_counters.extra_copy_events++;
            break;
        default:
            break;
    }
}

void bh_translation_record_extra_copy(uint64_t bytes_copied) {
    bh_translation_event_record(BH_TRANSLATION_EVENT_EXTRA_COPY);
    g_translation_counters.extra_copy_bytes += bytes_copied;
}

void bh_translation_counters_snapshot(bh_translation_counters_t* out) {
    if (!out) {
        return;
    }

    *out = g_translation_counters;
}

void bh_translation_counters_reset(void) {
    g_translation_counters = (bh_translation_counters_t){0};
}
