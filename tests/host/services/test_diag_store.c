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

int main() {
    test_store_and_retrieve();
    printf("test_diag_store passed.\n");
    return 0;
}
