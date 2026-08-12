#include "../../../core/services/core/init/init_rollback.h"
#include <assert.h>

static unsigned rollback_order[2];
static unsigned rollback_count;

static int record_rollback(void *ctx) {
    rollback_order[rollback_count++] = *(const unsigned *)ctx;
    return 0;
}

int main(void) {
    init_rollback_stack_t stack = {0};
    init_service_runtime_t services[INIT_SERVICE_ID_MAX] = {0};
    const unsigned first = 1;
    const unsigned second = 2;

    services[INIT_SVC_NAMESVC].state = INIT_SERVICE_STATE_READY;
    services[INIT_SVC_PROCESS_MANAGER].state = INIT_SERVICE_STATE_READY;
    assert(init_rollback_record(&stack, INIT_SVC_NAMESVC,
                                record_rollback, (void *)&first) == 0);
    assert(init_rollback_record(&stack, INIT_SVC_PROCESS_MANAGER,
                                record_rollback, (void *)&second) == 0);
    assert(init_rollback_run(&stack, services) == 0);
    assert(rollback_count == 2);
    assert(rollback_order[0] == 2 && rollback_order[1] == 1);
    assert(services[INIT_SVC_NAMESVC].state == INIT_SERVICE_STATE_DISABLED);
    assert(services[INIT_SVC_PROCESS_MANAGER].state == INIT_SERVICE_STATE_DISABLED);
    return 0;
}
