/* SPDX-License-Identifier: MIT */
#ifndef BHARAT_UAPI_DIAG_EVENT_H
#define BHARAT_UAPI_DIAG_EVENT_H

#include <stddef.h>
#include <stdint.h>
#include "version.h"

typedef enum bh_diag_severity {
    BH_DIAG_SEVERITY_TRACE = 0, BH_DIAG_SEVERITY_INFO, BH_DIAG_SEVERITY_NOTICE,
    BH_DIAG_SEVERITY_WARNING, BH_DIAG_SEVERITY_RECOVERABLE,
    BH_DIAG_SEVERITY_CRITICAL, BH_DIAG_SEVERITY_FATAL,
    BH_DIAG_SEVERITY_COUNT
} bh_diag_severity_t;

typedef enum bh_diag_source_kind {
    BH_DIAG_SOURCE_KERNEL = 0, BH_DIAG_SOURCE_SERVICE, BH_DIAG_SOURCE_DRIVER,
    BH_DIAG_SOURCE_STACK, BH_DIAG_SOURCE_RUNTIME, BH_DIAG_SOURCE_BUILD_TOOL,
    BH_DIAG_SOURCE_TEST_HARNESS, BH_DIAG_SOURCE_KIND_COUNT
} bh_diag_source_kind_t;

typedef enum bh_diag_subsystem {
    BH_DIAG_SUBSYSTEM_BOOT = 0, BH_DIAG_SUBSYSTEM_SCHEDULER,
    BH_DIAG_SUBSYSTEM_MEMORY, BH_DIAG_SUBSYSTEM_IPC,
    BH_DIAG_SUBSYSTEM_CAPABILITY, BH_DIAG_SUBSYSTEM_PROCESS,
    BH_DIAG_SUBSYSTEM_SERVICE, BH_DIAG_SUBSYSTEM_DRIVER,
    BH_DIAG_SUBSYSTEM_NETWORK, BH_DIAG_SUBSYSTEM_DISPLAY,
    BH_DIAG_SUBSYSTEM_SECURITY, BH_DIAG_SUBSYSTEM_POWER,
    BH_DIAG_SUBSYSTEM_WATCHDOG, BH_DIAG_SUBSYSTEM_UNKNOWN = 0xffff
} bh_diag_subsystem_t;

typedef struct bh_diag_event_header {
    uint16_t abi_version;
    uint16_t header_size;
    uint16_t event_type;
    uint8_t severity;
    uint8_t source_kind;
    uint32_t payload_size;
    uint32_t flags;
    uint64_t sequence;
    uint64_t timestamp_ns;
    uint32_t source_id;
    uint16_t cpu_id;
    uint16_t subsystem_id;
} bh_diag_event_header_t;

_Static_assert(sizeof(bh_diag_event_header_t) == 40, "diagnostic header ABI size");
_Static_assert(offsetof(bh_diag_event_header_t, sequence) == 16, "sequence ABI offset");
_Static_assert(offsetof(bh_diag_event_header_t, timestamp_ns) == 24, "timestamp ABI offset");
_Static_assert(offsetof(bh_diag_event_header_t, source_id) == 32, "source ABI offset");

#endif
