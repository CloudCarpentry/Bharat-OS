#ifndef BHARAT_INIT_RUNTIME_H
#define BHARAT_INIT_RUNTIME_H

#include "init_profile.h"
#include "init_status.h"

#define MAX_INIT_SERVICES 32

typedef struct {
    init_service_runtime_t services[MAX_INIT_SERVICES];
    size_t count;
    init_boot_context_t *ctx;
} init_runtime_ctx_t;

int init_runtime_run(init_boot_context_t *ctx);

#endif // BHARAT_INIT_RUNTIME_H
