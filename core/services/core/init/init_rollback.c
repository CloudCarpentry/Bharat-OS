#include "init_rollback.h"
#include <errno.h>

int init_rollback_record(init_rollback_stack_t *stack,
                         init_service_id_t id,
                         int (*rollback)(void *ctx), void *ctx) {
    if (!stack || !rollback || id <= INIT_SVC_NONE || id >= INIT_SERVICE_ID_MAX) {
        return -EINVAL;
    }
    if (stack->count >= INIT_ROLLBACK_MAX_RECORDS) return -ENOSPC;
    stack->records[stack->count++] =
        (init_rollback_record_t){ .id = id, .rollback = rollback, .ctx = ctx };
    return 0;
}

int init_rollback_run(init_rollback_stack_t *stack,
                      init_service_runtime_t services[INIT_SERVICE_ID_MAX]) {
    if (!stack || !services) return -EINVAL;

    int first_error = 0;
    while (stack->count != 0U) {
        init_rollback_record_t *record = &stack->records[--stack->count];
        int rc = record->rollback(record->ctx);
        if (rc == 0) {
            services[record->id].state = INIT_SERVICE_STATE_DISABLED;
            services[record->id].observed_ready = false;
        } else {
            /* Failed compensation is visible and fails closed. */
            services[record->id].state = INIT_SERVICE_STATE_FAILED;
            services[record->id].last_error = rc;
            if (first_error == 0) first_error = rc;
        }
    }
    return first_error;
}
