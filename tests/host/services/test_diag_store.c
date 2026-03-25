#include <stdio.h>
#include <assert.h>
#include "services/diag/diag_event.h"

// internal reset
void diag_store_reset(void);

void test_store_and_retrieve() {
    diag_store_reset();
    diag_event_t event1 = { .dtc = 0x1234, .severity = DIAG_SEV_WARN, .timestamp_ns = 1000 };
    diag_event_t event2 = { .dtc = 0x5678, .severity = DIAG_SEV_CRITICAL, .timestamp_ns = 2000 };

    assert(diag_report_event(&event1) == 0);
    assert(diag_report_event(&event2) == 0);

    diag_event_t out;
    assert(diag_get_event(0x1234, &out) == 0);
    assert(out.severity == DIAG_SEV_WARN);

    assert(diag_clear_event(0x1234) == 0);
    assert(diag_get_event(0x1234, &out) == -1); // Should be gone

    assert(diag_get_event(0x5678, &out) == 0); // Second event still there
}

void test_clear_scenarios() {
    diag_store_reset();
    diag_event_t e1 = { .dtc = 1, .severity = DIAG_SEV_INFO, .timestamp_ns = 100 };
    diag_event_t e2 = { .dtc = 2, .severity = DIAG_SEV_WARN, .timestamp_ns = 200 };
    diag_event_t e3 = { .dtc = 3, .severity = DIAG_SEV_CRITICAL, .timestamp_ns = 300 };
    diag_event_t e4 = { .dtc = 4, .severity = DIAG_SEV_DEGRADED, .timestamp_ns = 400 };
    diag_event_t out;

    // Remove non-existent item (from empty)
    assert(diag_clear_event(99) == -1);

    // Setup 1 item, remove only item
    assert(diag_report_event(&e1) == 0);
    assert(diag_clear_event(1) == 0);
    assert(diag_get_event(1, &out) == -1);

    // Setup 4 items
    assert(diag_report_event(&e1) == 0);
    assert(diag_report_event(&e2) == 0);
    assert(diag_report_event(&e3) == 0);
    assert(diag_report_event(&e4) == 0);

    // Remove from beginning (1)
    assert(diag_clear_event(1) == 0);
    assert(diag_get_event(1, &out) == -1);
    assert(diag_get_event(2, &out) == 0);
    assert(diag_get_event(3, &out) == 0);
    assert(diag_get_event(4, &out) == 0);

    // Remove from middle (3)
    assert(diag_clear_event(3) == 0);
    assert(diag_get_event(3, &out) == -1);
    assert(diag_get_event(2, &out) == 0);
    assert(diag_get_event(4, &out) == 0);

    // Remove from end (4, assuming swap has made it such or not, we know 4 and 2 are there)
    assert(diag_clear_event(4) == 0);
    assert(diag_get_event(4, &out) == -1);
    assert(diag_get_event(2, &out) == 0);

    // Remove remaining items until empty
    assert(diag_clear_event(2) == 0);
    assert(diag_get_event(2, &out) == -1);

    // Remove non-existent item
    assert(diag_clear_event(99) == -1);
}

int main() {
    test_store_and_retrieve();
    test_clear_scenarios();
    printf("test_diag_store passed.\n");
    return 0;
}
