#ifndef BHARAT_INPUTMGR_H
#define BHARAT_INPUTMGR_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/input/input_driver.h>
#include "bharat/uapi/input/input_event.h"

#define INPUTMGR_RING_CAPACITY 256

typedef struct {
    uint64_t sequence;
    uint64_t overflow_count;
    uint64_t malformed_count;
    uint64_t reset_count;
    uint32_t high_water_mark;
} bh_inputmgr_counters_t;

void inputmgr_init(void);

/**
 * Enqueues a raw event from any input device driver, performing normalization and coalescing.
 */
int bh_inputmgr_enqueue(uint32_t device_id, uint16_t type, uint16_t code, int32_t value);

/**
 * Consumes/dequeues events from the global normalized ring buffer.
 * Safe to call from non-IRQ context (such as the FBUI event loop).
 */
int bh_inputmgr_drain(bh_input_event_t *out_events, int max_events);

/**
 * Retrieves the current telemetry and counter statistics.
 */
const bh_inputmgr_counters_t *bh_inputmgr_get_counters(void);

/**
 * Resets the input manager's ring buffer and counters.
 */
void bh_inputmgr_reset(void);

#endif // BHARAT_INPUTMGR_H
