#include "init_handoff.h"
#include <kernel/status.h>
#include <bharat_config.h>

int init_handoff_to_supervisor(const init_boot_context_t *ctx) {
    if (!ctx) {
        return K_ERR_INVALID_ARG;
    }

#if defined(BHARAT_INIT_ENABLE_HANDOFF)
    // Future integration point for handing over to a full servicemgr/supervisor
    return K_OK;
#else
    // Currently, handoff is not supported by default, fulfilling the stub boundary
    return K_ERR_UNSUPPORTED;
#endif
}
