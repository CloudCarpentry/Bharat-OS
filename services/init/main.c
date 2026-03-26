#include <bharat/runtime/runtime.h>
#include <bharat/cap/cap.h>
#include "sysmgr.h"

int services_init_main(void);

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return services_init_main();
}

int services_init_main(void) {
    bharat_runtime_init();

    bharat_runtime_log("services/init: Starting user-space bootstrap (sysmgr).");

    // TODO: Intake root bootstrap capability from kernel/environment
    bharat_cap_handle_t root_cap = bharat_runtime_get_bootstrap_cap();
    if (!bharat_cap_is_valid(root_cap)) {
        bharat_runtime_log("services/init: Warning - no valid root bootstrap capability found.");
        // We do not panic yet because the kernel boot ABI might not supply this perfectly.
    } else {
        bharat_runtime_log("services/init: Bootstrap capability acquired.");
    }

    // TODO: Connect to or spawn services/namesvc
    // TODO: Spawn services/capmgr and delegate subset of root rights

    // Enforce profile matrix and start registered services
    sysmgr_enforce_startup_policy();

    bharat_runtime_log("services/init: Initialization graph complete. Suspending.");

    // Simulate main event loop or sleep
    while(1) {
        // Sleep or wait for supervisor IPC requests
    }

    // Unreachable
    bharat_runtime_shutdown();
    return 0;
}
