#include "init_handoff.h"
#include "init_contract.h"
#include <bharat/runtime/runtime.h>
#include <errno.h>

int init_handoff_to_supervisor(const init_boot_context_t *ctx, struct init_runtime_s *rt) {
    if (!ctx || !rt) return -EINVAL;
    /* The current request/reply protocol transfers its sole capability as a
     * reply SEND endpoint, not bootstrap authority. It also exposes no trusted
     * caller identity. Sending a bootstrap handle in the header proves nothing.
     * Keep init's active graph and lifecycle ownership until this is resolved.
     * launch.process_id is real and must not be replaced with zero; incarnation
     * identity is not yet returned by the bootstrap launch contract. */
    bharat_runtime_log("services/init: supervisor handoff unavailable: authenticated transport and incarnation identity required\n");
    return -ENOSYS;
}
