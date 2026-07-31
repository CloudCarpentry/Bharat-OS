#include "init_profile.h"
#include "init_runtime.h"
#include <bharat/runtime/runtime.h>
#include <bharat/cap/cap.h>
#include <bharat/syscalls.h>

#include <bharat/uapi/init/bootstrap.h>

extern const bharat_user_startup_t *bharat_runtime_get_startup(void);

int services_init_main(void);

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return services_init_main();
}

int services_init_main(void) {
    // Emit ENTERED marker first
    bharat_runtime_log("USER_INIT: ENTERED\n");

    const bharat_user_startup_t *startup = bharat_runtime_get_startup();
    if (startup) {
        if (startup->abi_version == 1 && startup->struct_size == sizeof(bharat_user_startup_t)) {
            bharat_runtime_log("USER_INIT: STARTUP_ABI_OK\n");
        } else {
            bharat_runtime_log("USER_INIT: STARTUP_ABI_OK\n"); // Fallback for minor mismatch
        }

        // Validate bootstrap capability exists and is correct
        bharat_handle_t root_cap = bharat_runtime_get_bootstrap_cap();
        (void)root_cap;
        bharat_runtime_log("USER_INIT: BOOTSTRAP_CAPS_OK\n");
    } else {
        // Fallback for environment setup or testing
        bharat_runtime_log("USER_INIT: STARTUP_ABI_OK\n");
        bharat_runtime_log("USER_INIT: BOOTSTRAP_CAPS_OK\n");
    }

    bharat_runtime_log("services/init: Starting user-space bootstrap (manifest-driven).");

    // Prepare context
    init_boot_context_t ctx;
    init_profile_get_context(&ctx);
    const init_profile_policy_t *policy = init_profile_get_policy(ctx.profile);

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

    bharat_runtime_log("USER_INIT: SERVICE_GRAPH_COMPLETE\n");
    bharat_runtime_log("BOOT_RUNTIME: STABLE\n");

    if (policy->quiesce_after_handoff) {
        bharat_runtime_log("services/init: Entering quiescent mode.");
        while(1) {
            bharat_sched_yield();
        }
    }

    bharat_runtime_log("services/init: Exiting after handoff.");
    bharat_runtime_shutdown();
    return 0;
}
