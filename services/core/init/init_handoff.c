#include "init_handoff.h"
#include <bharat/runtime/runtime.h>
#include <errno.h>

int init_handoff_to_supervisor(const init_boot_context_t *ctx) {
#ifdef BHARAT_INIT_ENABLE_HANDOFF
    bharat_runtime_log("services/init: Handing off to servicemgr supervisor...");
    // Future integration point for servicemgr
    return 0;
#else
    (void)ctx;
    bharat_runtime_log("services/init: Supervisor handoff not enabled. Idling.");
    return -ENOSYS;
#endif
}
