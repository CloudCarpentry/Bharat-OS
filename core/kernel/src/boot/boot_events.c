/* SPDX-License-Identifier: MIT */
#include "boot/boot_events.h"
#include "display/boot_gui_init.h"
#include <stdbool.h>
#include <stddef.h>

#if defined(BHARAT_KERNEL) || defined(BHARAT_HOST_TEST)
#include "hal/hal_timer.h"
#endif

static bh_boot_event_record_t g_boot_ring[BH_BOOT_EVENT_RING_CAPACITY];
static uint32_t g_total_events = 0;
static uint32_t g_head_idx = 0;

static void safe_str_copy(char *dest, const char *src, size_t max_len) {
    if (!dest || max_len == 0) return;
    size_t i = 0;
    if (src) {
        while (src[i] != '\0' && i + 1 < max_len) {
            dest[i] = src[i];
            i++;
        }
    }
    dest[i] = '\0';
}

void boot_events_init(void) {
    g_total_events = 0;
    g_head_idx = 0;
    for (size_t i = 0; i < BH_BOOT_EVENT_RING_CAPACITY; ++i) {
        g_boot_ring[i].timestamp_ns = 0;
        g_boot_ring[i].stage = BH_BOOT_STAGE_EARLY;
        g_boot_ring[i].status = BH_BOOT_STATUS_PENDING;
        g_boot_ring[i].error_code = 0;
        g_boot_ring[i].component[0] = '\0';
        g_boot_ring[i].message[0] = '\0';
    }
}

void boot_events_record(bh_boot_stage_t stage,
                        bh_boot_status_t status,
                        const char *component,
                        const char *message,
                        int32_t error_code) {
    uint64_t ts = 0;
#if defined(BHARAT_KERNEL) || defined(BHARAT_HOST_TEST)
    if (!hal_timer_monotonic_ns(&ts)) {
        ts = hal_timer_read_ns();
    }
#endif

    uint32_t slot = g_head_idx % BH_BOOT_EVENT_RING_CAPACITY;
    bh_boot_event_record_t *rec = &g_boot_ring[slot];

    rec->timestamp_ns = ts;
    rec->stage = stage;
    rec->status = status;
    rec->error_code = error_code;
    safe_str_copy(rec->component, component, sizeof(rec->component));
    safe_str_copy(rec->message, message, sizeof(rec->message));

    g_head_idx = (g_head_idx + 1) % BH_BOOT_EVENT_RING_CAPACITY;
    g_total_events++;
}

void boot_events_publish(bh_boot_stage_t stage, uint8_t percent, bharat_status_t status, const char *label) {
    bh_boot_status_t st = (status == BHARAT_STATUS_OK) ? BH_BOOT_STATUS_OK : BH_BOOT_STATUS_ERROR;
    boot_events_record(stage, st, "KERNEL", label, (int32_t)status);
#if defined(BHARAT_KERNEL)
    boot_gui_update_progress(percent, label);
#else
    (void)percent;
#endif
}

void bh_boot_events_get_snapshot(bh_boot_event_snapshot_t *snapshot) {
    if (!snapshot) return;

    snapshot->total_events = g_total_events;
    if (g_total_events > BH_BOOT_EVENT_RING_CAPACITY) {
        snapshot->dropped_events = g_total_events - BH_BOOT_EVENT_RING_CAPACITY;
        snapshot->count = BH_BOOT_EVENT_RING_CAPACITY;
    } else {
        snapshot->dropped_events = 0;
        snapshot->count = g_total_events;
    }

    uint32_t start_idx = 0;
    if (g_total_events > BH_BOOT_EVENT_RING_CAPACITY) {
        start_idx = g_head_idx; /* Oldest available entry */
    }

    for (uint32_t i = 0; i < snapshot->count; ++i) {
        uint32_t slot = (start_idx + i) % BH_BOOT_EVENT_RING_CAPACITY;
        snapshot->events[i] = g_boot_ring[slot];
    }
}

const char *bh_boot_stage_name(bh_boot_stage_t stage) {
    switch (stage) {
        case BH_BOOT_STAGE_EARLY: return "EARLY";
        case BH_BOOT_STAGE_HAL: return "HAL";
        case BH_BOOT_STAGE_SECURITY: return "SECURITY";
        case BH_BOOT_STAGE_MEMORY: return "MEMORY";
        case BH_BOOT_STAGE_SCHEDULER: return "SCHEDULER";
        case BH_BOOT_STAGE_DRIVERS: return "DRIVERS";
        case BH_BOOT_STAGE_SERVICES: return "SERVICES";
        case BH_BOOT_STAGE_USERSPACE: return "USERSPACE";
        case BH_BOOT_STAGE_READY: return "READY";
        case BH_BOOT_STAGE_FAILURE: return "FAILURE";
        default: return "UNKNOWN";
    }
}

const char *bh_boot_status_name(bh_boot_status_t status) {
    switch (status) {
        case BH_BOOT_STATUS_PENDING: return "PENDING";
        case BH_BOOT_STATUS_IN_PROGRESS: return "IN_PROGRESS";
        case BH_BOOT_STATUS_OK: return "OK";
        case BH_BOOT_STATUS_WARNING: return "WARNING";
        case BH_BOOT_STATUS_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}
