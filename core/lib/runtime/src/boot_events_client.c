/* SPDX-License-Identifier: MIT */
#include "bharat/uapi/boot/boot_events.h"
#include <bharat/uapi/service_status.h>
#include <bharat/uapi/init/bootstrap.h>
#include <bharat/uapi/syscall_nr.h>
#include <bharat/uapi/syscall/bh_syscall.h>
#include <bharat/syscalls.h>
#include <bharat/runtime/runtime.h>
#include <string.h>

#ifndef BH_SYS_BOOT_GET_EVENTS
#define BH_SYS_BOOT_GET_EVENTS 25
#endif

bharat_status_t bh_boot_events_fetch_snapshot(uint32_t diag_cap, bh_boot_event_snapshot_t *snapshot) {
    if (!snapshot) {
        return BHARAT_STATUS_ERR_INVALID_ARG;
    }
    if (diag_cap == 0) {
        return BHARAT_STATUS_ERR_PERMISSION;
    }
    memset(snapshot, 0, sizeof(*snapshot));
    int res = bharat_syscall(BH_SYS_BOOT_GET_EVENTS, (uintptr_t)diag_cap, (uintptr_t)snapshot, 0, 0, 0, 0);
    if (res != 0) {
        return (res < 0) ? (bharat_status_t)res : BHARAT_STATUS_ERR_PERMISSION;
    }
    return BHARAT_STATUS_OK;
}

void bh_boot_events_get_snapshot(bh_boot_event_snapshot_t *snapshot) {
    if (!snapshot) return;
    memset(snapshot, 0, sizeof(*snapshot));
    const bharat_user_startup_t *startup = bharat_runtime_get_startup();
    uint32_t cap = startup ? (uint32_t)startup->bootstrap.self_process_cap : 0;
    (void)bh_boot_events_fetch_snapshot(cap, snapshot);
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
