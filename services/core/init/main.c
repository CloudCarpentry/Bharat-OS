#include <bharat/runtime/runtime.h>
#include <bharat/cap/cap.h>
#include <kernel/status.h>
#include "sysmgr.h"
#include "init_runtime.h"

int services_init_main(void);

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return services_init_main();
}

int services_init_main(void) {
    bharat_runtime_init();

    bharat_runtime_log("services/init: Starting user-space bootstrap.");

    // TODO: Intake root bootstrap capability from kernel/environment
    bharat_cap_handle_t root_cap = bharat_runtime_get_bootstrap_cap();
    if (!bharat_cap_is_valid(root_cap)) {
        bharat_runtime_log("services/init: Warning - no valid root bootstrap capability found.");
        // We do not panic yet because the kernel boot ABI might not supply this perfectly.
    } else {
        bharat_runtime_log("services/init: Bootstrap capability acquired.");
    }

    init_boot_context_t ctx = {
        .profile = init_profile_get_active(),
        .arch_id = 0,
        .platform_id = 0,
        .board_id = 0,
        .personality_id = 0,
        .cap_mask = ~0ULL,
        .safe_mode = false,
        .diagnostics_mode = false,
    };

    // Enforce profile matrix and start registered services using the new manifest logic
    init_runtime_start(&ctx);

    bharat_runtime_log("services/init: Initialization graph complete.");

    // Attempt to hand off to supervisor
    int handoff_res = init_handoff_to_supervisor(&ctx);
    if (handoff_res != K_OK) {
        bharat_runtime_log("services/init: Handoff skipped or unsupported. Suspending in generic loop.");
        // Simulate main event loop or sleep
        while(1) {
            // Sleep or wait for supervisor IPC requests
        }
    }

    // Unreachable if handoff exits process, but wait just in case
    while(1) {}

    bharat_runtime_shutdown();
    return 0;
}
