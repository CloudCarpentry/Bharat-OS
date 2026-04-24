#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include "services/diag/diag_event.h"

// internal resets and declarations
void diag_store_reset(void);
void diag_debounce_reset(void);
int diag_debounce_report(const diag_event_t* event, bool is_fault);

void test_transient_debounce() {
    diag_store_reset();
    diag_debounce_reset();

    diag_event_t transient_event = { .dtc = 0x1000, .severity = DIAG_SEV_WARN };

    // 1 fault shouldn't trigger store
    assert(diag_debounce_report(&transient_event, true) == 0);
    diag_event_t out;
    assert(diag_get_event(0x1000, &out) == -1);

    // 2 more faults
    assert(diag_debounce_report(&transient_event, true) == 0);
    assert(diag_debounce_report(&transient_event, true) == 0);
    // Still not 3
    assert(diag_get_event(0x1000, &out) == -1);

    // 4th fault triggers it
    assert(diag_debounce_report(&transient_event, true) == 0);
    assert(diag_get_event(0x1000, &out) == 0);
}

void test_critical_immediate() {
    diag_store_reset();
    diag_debounce_reset();

    diag_event_t critical_event = { .dtc = 0x2000, .severity = DIAG_SEV_CRITICAL };

    // First fault immediately stored for critical
    assert(diag_debounce_report(&critical_event, true) == 0);
    diag_event_t out;
    assert(diag_get_event(0x2000, &out) == 0);
}

int main() {
    test_transient_debounce();
    test_critical_immediate();
    printf("test_diag_debounce passed.\n");
    return 0;
}
