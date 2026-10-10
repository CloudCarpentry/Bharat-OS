/* SPDX-License-Identifier: MIT */
#ifndef BHARAT_UAPI_BOOT_EVENTS_H
#define BHARAT_UAPI_BOOT_EVENTS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <bharat/uapi/service_status.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BH_BOOT_STAGE_EARLY = 0,
    BH_BOOT_STAGE_HAL,
    BH_BOOT_STAGE_SECURITY,
    BH_BOOT_STAGE_MEMORY,
    BH_BOOT_STAGE_SCHEDULER,
    BH_BOOT_STAGE_DRIVERS,
    BH_BOOT_STAGE_SERVICES,
    BH_BOOT_STAGE_USERSPACE,
    BH_BOOT_STAGE_READY,
    BH_BOOT_STAGE_FAILURE,
    BH_BOOT_STAGE_COUNT
} bh_boot_stage_t;

typedef enum {
    BH_BOOT_STATUS_PENDING = 0,
    BH_BOOT_STATUS_IN_PROGRESS,
    BH_BOOT_STATUS_OK,
    BH_BOOT_STATUS_WARNING,
    BH_BOOT_STATUS_ERROR
} bh_boot_status_t;

typedef struct {
    uint64_t timestamp_ns;
    bh_boot_stage_t stage;
    bh_boot_status_t status;
    int32_t error_code;
    char component[32];
    char message[96];
} bh_boot_event_record_t;

#define BH_BOOT_EVENT_RING_CAPACITY 32

typedef struct {
    uint32_t total_events;
    uint32_t dropped_events;
    uint32_t count;
    bh_boot_event_record_t events[BH_BOOT_EVENT_RING_CAPACITY];
} bh_boot_event_snapshot_t;

/* Backward compatible single event definition */
typedef struct {
    bh_boot_stage_t stage;
    uint8_t percent;
    bharat_status_t status;
    char label[128];
} bh_boot_event_t;

/* Public snapshot and status query interface */
void bh_boot_events_get_snapshot(bh_boot_event_snapshot_t *snapshot);
const char *bh_boot_stage_name(bh_boot_stage_t stage);
const char *bh_boot_status_name(bh_boot_status_t status);

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_UAPI_BOOT_EVENTS_H */
