#ifndef BHARAT_INIT_STATUS_H
#define BHARAT_INIT_STATUS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Pre-declaration to break dependency loop with init_manifest.h
struct init_service_desc_s;

typedef enum {
    INIT_FAIL_NONE = 0,
    INIT_FAIL_DEP,
    INIT_FAIL_TIMEOUT,
    INIT_FAIL_LAUNCH,
    INIT_FAIL_CAPABILITY,
    INIT_FAIL_PROFILE,
    INIT_FAIL_HEARTBEAT,
    INIT_FAIL_HANDOFF,
} init_failure_class_t;

typedef enum {
    INIT_BOOT_OUTCOME_SUCCESS = 0,
    INIT_BOOT_OUTCOME_DEGRADED,
    INIT_BOOT_OUTCOME_SAFE_MODE,
    INIT_BOOT_OUTCOME_HANDOFF_FAILED,
    INIT_BOOT_OUTCOME_BOOTSTRAP_DEFERRED,
} init_boot_outcome_t;

typedef enum {
    INIT_PHASE_RESET = 0,
    INIT_PHASE_CONTEXT_READY,
    INIT_PHASE_KERNEL_HEALTH_VALIDATED,
    INIT_PHASE_PROFILE_SELECTED,
    INIT_PHASE_PERSONALITY_SELECTED,
    INIT_PHASE_MANIFEST_SELECTED,
    INIT_PHASE_GRAPH_VALIDATED,
    INIT_PHASE_CORE_STARTING,
    INIT_PHASE_CORE_READY,
    INIT_PHASE_INFRA_STARTING,
    INIT_PHASE_INFRA_READY,
    INIT_PHASE_OPTIONAL_STARTING,
    INIT_PHASE_HANDOFF_PREPARED,
    INIT_PHASE_HANDOFF_COMPLETE,
    INIT_PHASE_QUIESCENT,
} init_phase_t;

typedef enum {
    BOOT_CLASS_CORE = 0,
    BOOT_CLASS_INFRA,
    BOOT_CLASS_OPTIONAL,
    BOOT_CLASS_LATE,
    BOOT_CLASS_DIAGNOSTIC,
} init_boot_class_t;

typedef enum {
    INIT_SERVICE_STATE_DISABLED = 0,
    INIT_SERVICE_STATE_PENDING,
    INIT_SERVICE_STATE_WAITING_DEPS,
    INIT_SERVICE_STATE_LAUNCH_REQUESTED,
    INIT_SERVICE_STATE_REGISTERED,
    INIT_SERVICE_STATE_READY,
    INIT_SERVICE_STATE_FAILED,
    INIT_SERVICE_STATE_SKIPPED,
    // Aliases to avoid breaking older code right away:
    INIT_SERVICE_STOPPED = INIT_SERVICE_STATE_PENDING,
    INIT_SERVICE_STARTING = INIT_SERVICE_STATE_LAUNCH_REQUESTED,
    INIT_SERVICE_RUNNING = INIT_SERVICE_STATE_READY,
} init_service_state_t;

typedef struct {
    const struct init_service_desc_s *desc;
    init_service_state_t state;
    uint8_t attempts;
    uint8_t retries_used;
    int last_error;
    bool required_for_boot;
    bool observed_registered;
    bool observed_ready;
} init_service_runtime_t;

void init_status_report(const init_service_runtime_t *runtimes, size_t count);

#endif // BHARAT_INIT_STATUS_H
