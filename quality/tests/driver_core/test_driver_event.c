#include <stdio.h>
#include <assert.h>
#include "event.h"
#include "device_registry.h"
#include "bharat/uapi/sys_errno.h"

static int callback_count = 0;
static void mock_callback(device_event_t* event) {
    callback_count++;
}

void test_event_accounting() {
    printf("Running test_event_accounting...\n");

    driver_event_stats_t stats;
    driver_event_get_stats(&stats);
    uint64_t initial_emitted = stats.emitted_count;

    device_desc_t dev = { .name = "event-dev" };

    // Register listener
    callback_count = 0;
    assert(driver_event_register_listener(mock_callback) == 0);

    // Emit event
    driver_event_emit(EVENT_DEVICE_ADDED, &dev);
    assert(callback_count == 1);

    driver_event_get_stats(&stats);
    assert(stats.emitted_count == initial_emitted + 1);
    assert(stats.delivered_count > 0);
    assert(stats.dropped_count == 0);

    // Emit event with NULL device (should be dropped)
    driver_event_emit(EVENT_DEVICE_ADDED, NULL);
    driver_event_get_stats(&stats);
    assert(stats.dropped_count == 1);

    printf("test_event_accounting passed!\n");
}

int main() {
    test_event_accounting();
    printf("All event accounting tests passed!\n");
    return 0;
}
