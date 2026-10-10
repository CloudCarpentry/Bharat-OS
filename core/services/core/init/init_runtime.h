#ifndef BHARAT_INIT_RUNTIME_H
#define BHARAT_INIT_RUNTIME_H

#include "init_profile.h"
#include "init_status.h"
#include "init_contract.h"

#define MAX_INIT_SERVICES INIT_SERVICE_ID_MAX

#define INIT_RUNTIME_HANDOFF_COMPLETE 0
#define INIT_RUNTIME_QUIESCENT 1
#define INIT_RUNTIME_RETAINED 2

int init_service_apply_event(init_service_runtime_t *sr, const bh_bootstrap_service_event_t *event);

int init_runtime_run(init_boot_context_t *ctx);

#endif // BHARAT_INIT_RUNTIME_H
