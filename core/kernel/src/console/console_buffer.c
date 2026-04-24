#include "console/console_buffer.h"

static console_ring_t g_console_ring;

void console_buffer_init(void) {
    g_console_ring.head = 0;
    g_console_ring.count = 0;
    g_console_ring.next_seq = 0;
    g_console_ring.dropped_records = 0;
}

bool console_buffer_push(console_record_t *rec) {
    if (!rec) return false;

    rec->seq_no = g_console_ring.next_seq++;

    console_index_t idx = g_console_ring.head;
    g_console_ring.records[idx] = *rec;

    g_console_ring.head = (idx + 1) % CONSOLE_RING_CAPACITY;

    if (g_console_ring.count < CONSOLE_RING_CAPACITY) {
        g_console_ring.count++;
    } else {
        g_console_ring.dropped_records++;
    }

    return true;
}

bool console_buffer_peek_oldest(console_record_t *out, console_index_t logical_index) {
    if (!out || g_console_ring.count == 0) return false;

    // logical_index represents an index in the range [0, g_console_ring.count)
    if (logical_index >= g_console_ring.count) return false;

    console_index_t oldest_idx;
    if (g_console_ring.count < CONSOLE_RING_CAPACITY) {
        oldest_idx = 0;
    } else {
        oldest_idx = g_console_ring.head;
    }

    console_index_t physical_idx = (oldest_idx + logical_index) % CONSOLE_RING_CAPACITY;
    *out = g_console_ring.records[physical_idx];

    return true;
}

size_t console_buffer_replay_to_backend(struct console_backend *backend, bool include_below_min) {
    if (!backend || !backend->ops || !backend->ops->write) return 0;

    size_t replayed = 0;
    for (console_index_t i = 0; i < g_console_ring.count; i++) {
        console_record_t rec;
        if (console_buffer_peek_oldest(&rec, i)) {
            if (!include_below_min && rec.level < backend->min_level) {
                continue;
            }
            backend->ops->write(backend, rec.text, rec.text_len);
            replayed++;
        }
    }
    return replayed;
}

uint32_t console_buffer_dropped_count(void) {
    return g_console_ring.dropped_records;
}
