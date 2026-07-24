#ifndef BHARAT_INIT_CONTRACT_H
#define BHARAT_INIT_CONTRACT_H

#include <stdbool.h>
#include <bharat/uapi/init/init_boot_context.h>
#include "init_manifest.h"
#include "init_events.h"
#include "init_handoff.h"

#define INIT_SERVICE_ID_MAX 64

typedef struct {
    init_boot_context_t boot_ctx;

    // Abstracting manifest info instead of full copy to avoid big stack/heap alloc
    size_t manifest_count;
    init_service_id_t service_order[INIT_SERVICE_ID_MAX];

    init_phase_t phase;
    init_failure_class_t failure_class;
    init_boot_outcome_t outcome;
    uint32_t safe_mode_reason;
    init_event_queue_t event_queue;
    init_service_runtime_t services[INIT_SERVICE_ID_MAX];
    bool handoff_ack_received;
} init_runtime_t;

int init_runtime_bootstrap(init_runtime_t *rt);

#endif // BHARAT_INIT_CONTRACT_H
