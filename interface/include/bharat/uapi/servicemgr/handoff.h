#ifndef BHARAT_UAPI_SERVICEMGR_HANDOFF_H
#define BHARAT_UAPI_SERVICEMGR_HANDOFF_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SM_HANDOFF_MAX_DEPS 4

typedef struct {
    uint32_t abi_major;
    uint32_t abi_minor;
    uint32_t struct_size;
    uint32_t flags;

    uint64_t boot_session_id;

    uint32_t selected_profile;
    uint32_t final_boot_phase;
    uint32_t boot_outcome;
    uint32_t failure_class;
    uint32_t safe_mode_reason;

    uint32_t kernel_health_flags;
    uint32_t selected_service_count;
    uint32_t already_running_count;
    uint32_t manifest_version;

    uint64_t manifest_hash;
} sm_handoff_begin_t;

typedef struct {
    uint64_t incarnation_id;
    uint32_t process_id;
    uint16_t record_index;
    uint16_t service_id;

    uint16_t executable_id;
    uint16_t start_deadline_ms;
    uint16_t ready_deadline_ms;
    uint8_t priority;
    uint8_t boot_class;

    uint8_t restart_policy;
    uint8_t retry_limit;
    uint8_t critical;
    uint8_t observed_state;

    uint8_t dependency_count;
    uint8_t dependencies[SM_HANDOFF_MAX_DEPS];
    char service_name[16];
    uint8_t reserved[3]; // Explicit padding for 8-byte alignment of the struct size
} sm_handoff_service_t;

_Static_assert(sizeof(sm_handoff_service_t) <= 72, "sm_handoff_service_t exceeds maximum IPC payload limit of 72 bytes");

typedef struct {
    uint64_t boot_session_id;
    uint32_t record_count;
    uint32_t required_ready;
    uint32_t required_failed;
    uint32_t optional_ready;
    uint32_t optional_failed;
    uint64_t records_hash;
} sm_handoff_commit_t;

typedef struct {
    int32_t status;
} sm_handoff_resp_t;

#ifdef __cplusplus
}
#endif

#endif // BHARAT_UAPI_SERVICEMGR_HANDOFF_H
