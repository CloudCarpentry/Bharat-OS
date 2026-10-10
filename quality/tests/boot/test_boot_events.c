/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "bharat/uapi/boot/boot_events.h"
#include "boot/boot_events.h"

/* Mock HAL timer for host testing */
static uint64_t g_mock_time_ns = 1000000ULL;

bool hal_timer_monotonic_ns(uint64_t *out_ns) {
    if (out_ns) {
        *out_ns = g_mock_time_ns;
    }
    return true;
}

uint64_t hal_timer_read_ns(void) {
    return g_mock_time_ns;
}

/* Mock GUI update */
static uint8_t g_last_percent = 0;
static char g_last_label[128] = {0};

void boot_gui_update_progress(uint8_t percent, const char *label) {
    g_last_percent = percent;
    if (label) {
        strncpy(g_last_label, label, sizeof(g_last_label) - 1);
        g_last_label[sizeof(g_last_label) - 1] = '\0';
    }
}

static void test_initial_state(void) {
    boot_events_init();
    bh_boot_event_snapshot_t snap;
    memset(&snap, 0xFF, sizeof(snap));

    bh_boot_events_get_snapshot(&snap);
    assert(snap.total_events == 0);
    assert(snap.dropped_events == 0);
    assert(snap.count == 0);
    printf("test_initial_state PASSED\n");
}

static void test_single_and_multiple_events(void) {
    boot_events_init();
    g_mock_time_ns = 5000000ULL;

    boot_events_record(BH_BOOT_STAGE_EARLY, BH_BOOT_STATUS_OK, "HAL", "Hardware init", 0);
    g_mock_time_ns += 1000000ULL;
    boot_events_record(BH_BOOT_STAGE_MEMORY, BH_BOOT_STATUS_OK, "PMM", "PMM ready", 0);

    bh_boot_event_snapshot_t snap;
    bh_boot_events_get_snapshot(&snap);

    assert(snap.total_events == 2);
    assert(snap.dropped_events == 0);
    assert(snap.count == 2);
    assert(snap.events[0].stage == BH_BOOT_STAGE_EARLY);
    assert(strcmp(snap.events[0].component, "HAL") == 0);
    assert(snap.events[0].timestamp_ns == 5000000ULL);
    assert(snap.events[1].stage == BH_BOOT_STAGE_MEMORY);
    assert(strcmp(snap.events[1].component, "PMM") == 0);
    assert(snap.events[1].timestamp_ns == 6000000ULL);
    printf("test_single_and_multiple_events PASSED\n");
}

static void test_ring_overflow_behavior(void) {
    boot_events_init();
    g_mock_time_ns = 1000ULL;

    /* Write 40 events to a capacity-32 ring */
    for (uint32_t i = 0; i < 40; ++i) {
        char msg[32];
        snprintf(msg, sizeof(msg), "event_%u", i);
        boot_events_record(BH_BOOT_STAGE_SERVICES, BH_BOOT_STATUS_OK, "SRV", msg, (int32_t)i);
        g_mock_time_ns += 1000ULL;
    }

    bh_boot_event_snapshot_t snap;
    bh_boot_events_get_snapshot(&snap);

    assert(snap.total_events == 40);
    assert(snap.dropped_events == 8);
    assert(snap.count == BH_BOOT_EVENT_RING_CAPACITY);

    /* First available event should be event 8 */
    assert(strcmp(snap.events[0].message, "event_8") == 0);
    assert(snap.events[0].error_code == 8);

    /* Last available event should be event 39 */
    assert(strcmp(snap.events[BH_BOOT_EVENT_RING_CAPACITY - 1].message, "event_39") == 0);
    assert(snap.events[BH_BOOT_EVENT_RING_CAPACITY - 1].error_code == 39);

    printf("test_ring_overflow_behavior PASSED\n");
}

static void test_legacy_publish_adapter(void) {
    boot_events_init();
    boot_events_publish(BH_BOOT_STAGE_SCHEDULER, 50, BHARAT_STATUS_OK, "SCHEDULER_ONLINE");

    assert(g_last_percent == 50);
    assert(strcmp(g_last_label, "SCHEDULER_ONLINE") == 0);

    bh_boot_event_snapshot_t snap;
    bh_boot_events_get_snapshot(&snap);
    assert(snap.count == 1);
    assert(snap.events[0].stage == BH_BOOT_STAGE_SCHEDULER);
    assert(snap.events[0].status == BH_BOOT_STATUS_OK);
    assert(strcmp(snap.events[0].message, "SCHEDULER_ONLINE") == 0);

    printf("test_legacy_publish_adapter PASSED\n");
}

static void test_enum_name_helpers(void) {
    assert(strcmp(bh_boot_stage_name(BH_BOOT_STAGE_MEMORY), "MEMORY") == 0);
    assert(strcmp(bh_boot_stage_name(BH_BOOT_STAGE_USERSPACE), "USERSPACE") == 0);
    assert(strcmp(bh_boot_status_name(BH_BOOT_STATUS_OK), "OK") == 0);
    assert(strcmp(bh_boot_status_name(BH_BOOT_STATUS_ERROR), "ERROR") == 0);
    printf("test_enum_name_helpers PASSED\n");
}

static void test_kernel_snapshot_direct(void) {
    boot_events_init();
    boot_events_record(BH_BOOT_STAGE_HAL, BH_BOOT_STATUS_OK, "HAL_TIMER", "Timer init ok", 0);
    boot_events_record(BH_BOOT_STAGE_MEMORY, BH_BOOT_STATUS_OK, "PMM", "4096 pages ready", 0);

    /* Null pointer safety check */
    kernel_boot_events_get_snapshot(NULL);
    bh_boot_events_get_snapshot(NULL);

    bh_boot_event_snapshot_t snap;
    memset(&snap, 0, sizeof(snap));
    kernel_boot_events_get_snapshot(&snap);

    assert(snap.total_events == 2);
    assert(snap.count == 2);
    assert(strcmp(snap.events[0].component, "HAL_TIMER") == 0);
    assert(strcmp(snap.events[1].component, "PMM") == 0);
    printf("test_kernel_snapshot_direct PASSED\n");
}

int main(void) {
    test_initial_state();
    test_single_and_multiple_events();
    test_ring_overflow_behavior();
    test_legacy_publish_adapter();
    test_enum_name_helpers();
    test_kernel_snapshot_direct();

    printf("All boot event pipeline tests PASSED!\n");
    return 0;
}
