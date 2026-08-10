/* SPDX-License-Identifier: MIT */
#ifndef BHARAT_UAPI_DIAG_HEALTH_H
#define BHARAT_UAPI_DIAG_HEALTH_H
#include <stdint.h>
typedef enum bh_diag_health_state { BH_DIAG_HEALTH_UNKNOWN = 0, BH_DIAG_HEALTH_STARTING, BH_DIAG_HEALTH_HEALTHY, BH_DIAG_HEALTH_DEGRADED, BH_DIAG_HEALTH_FAILED, BH_DIAG_HEALTH_STOPPING } bh_diag_health_state_t;
typedef struct bh_diag_health_snapshot { uint16_t schema_version; uint8_t state; uint8_t reserved; uint32_t source_id; uint64_t timestamp_ns; uint64_t uptime_ns; uint64_t dropped_events; uint64_t last_fault_sequence; uint32_t restart_count; uint32_t flags; } bh_diag_health_snapshot_t;
_Static_assert(sizeof(bh_diag_health_snapshot_t) == 48, "health ABI size");
#endif
