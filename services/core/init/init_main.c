#include "init_profile.h"
#include "init_runtime.h"
#include <bharat/runtime/runtime.h>
#include <bharat/cap/cap.h>

int services_init_main(void);

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return services_init_main();
}

int services_init_main(void) {
    bharat_runtime_init();

    bharat_runtime_log("services/init: Starting user-space bootstrap (manifest-driven).");

    // TODO: Intake root bootstrap capability from kernel/environment
    bharat_cap_handle_t root_cap = bharat_runtime_get_bootstrap_cap();
    if (!bharat_cap_is_valid(root_cap)) {
        bharat_runtime_log("services/init: Warning - no valid root bootstrap capability found.");
    } else {
        bharat_runtime_log("services/init: Bootstrap capability acquired.");
    }

    // Prepare context
    init_boot_context_t ctx;
    init_profile_get_context(&ctx);

    if (ctx.profile == INIT_PROFILE_TINY) {
        bharat_runtime_log("services/init: Running in TINY profile mode.");
    } else if (ctx.safe_mode_requested) {
         bharat_runtime_log("services/init: Booting in SAFE_MODE.");
    }

    // Run the startup sequence
    int result = init_runtime_run(&ctx);
    if (result != 0) {
        bharat_runtime_log("services/init: Bootstrap failed (safe mode / halted).");
        // Hang
        while (1) {
            bharat_sched_yield();
        }
    }

    bharat_runtime_log("services/init: Initialization graph complete. Suspending.");

    // Simulate main event loop or sleep
    while(1) {
        bharat_sched_yield();
    }

    // Unreachable
    bharat_runtime_shutdown();
    return 0;
}
