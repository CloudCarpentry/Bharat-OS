/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "bharat/uapi/boot/boot_events.h"
#include "boot/boot_events.h"
#include "experience/shell/bharat_shell.h"

/* Mock HAL timer for test */
bool hal_timer_monotonic_ns(uint64_t *out_ns) {
    if (out_ns) *out_ns = 12345678ULL;
    return true;
}

uint64_t hal_timer_read_ns(void) {
    return 12345678ULL;
}

void boot_gui_update_progress(uint8_t percent, const char *label) {
    (void)percent;
    (void)label;
}

static void test_diagnostics_snapshot_formatting(void) {
    boot_events_init();

    boot_events_record(BH_BOOT_STAGE_HAL, BH_BOOT_STATUS_OK, "HAL_TIMER", "Timer init ok", 0);
    boot_events_record(BH_BOOT_STAGE_MEMORY, BH_BOOT_STATUS_OK, "PMM", "4096 pages ready", 0);
    boot_events_record(BH_BOOT_STAGE_SERVICES, BH_BOOT_STATUS_WARNING, "DEVMGR", "Optional device missing", 2);
    boot_events_record(BH_BOOT_STAGE_USERSPACE, BH_BOOT_STATUS_OK, "SYSMGR", "Userspace active", 0);

    bh_boot_event_snapshot_t snap;
    bh_boot_events_get_snapshot(&snap);

    assert(snap.count == 4);
    assert(snap.total_events == 4);
    assert(snap.dropped_events == 0);

    assert(strcmp(bh_boot_stage_name(snap.events[0].stage), "HAL") == 0);
    assert(strcmp(snap.events[0].component, "HAL_TIMER") == 0);
    assert(snap.events[0].status == BH_BOOT_STATUS_OK);

    assert(strcmp(bh_boot_stage_name(snap.events[2].stage), "SERVICES") == 0);
    assert(snap.events[2].status == BH_BOOT_STATUS_WARNING);
    assert(snap.events[2].error_code == 2);

    assert(strcmp(bh_boot_stage_name(snap.events[3].stage), "USERSPACE") == 0);
    assert(snap.events[3].status == BH_BOOT_STATUS_OK);

    printf("test_diagnostics_snapshot_formatting PASSED\n");
}

static void test_screen_id_enumeration(void) {
    assert(BH_SHELL_SCREEN_SPLASH == 0);
    assert(BH_SHELL_SCREEN_LAUNCHER == 1);
    assert(BH_SHELL_SCREEN_SYSTEM == 2);
    assert(BH_SHELL_SCREEN_DEVICES == 3);
    assert(BH_SHELL_SCREEN_DIAGNOSTICS == 4);
    assert(BH_SHELL_SCREEN_DEMOS == 5);
    assert(BH_SHELL_SCREEN_COUNT == 6);
    printf("test_screen_id_enumeration PASSED\n");
}

int main(void) {
    test_diagnostics_snapshot_formatting();
    test_screen_id_enumeration();

    printf("All boot diagnostics tests PASSED!\n");
    return 0;
}
