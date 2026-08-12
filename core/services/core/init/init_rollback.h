#ifndef BHARAT_INIT_ROLLBACK_H
#define BHARAT_INIT_ROLLBACK_H

#include "init_manifest.h"

#define INIT_ROLLBACK_MAX_RECORDS INIT_SERVICE_ID_MAX

typedef struct {
    init_service_id_t id;
    int (*rollback)(void *ctx);
    void *ctx;
} init_rollback_record_t;

typedef struct {
    init_rollback_record_t records[INIT_ROLLBACK_MAX_RECORDS];
    size_t count;
} init_rollback_stack_t;

int init_rollback_record(init_rollback_stack_t *stack,
                         init_service_id_t id,
                         int (*rollback)(void *ctx), void *ctx);
int init_rollback_run(init_rollback_stack_t *stack,
                      init_service_runtime_t services[INIT_SERVICE_ID_MAX]);

#endif
