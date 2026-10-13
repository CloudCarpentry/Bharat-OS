#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <bharat/coremgr/coremgr.h>
#include <bharat/bh_native.h>

/**
 * @file main.c
 * @brief coremgr - Per-core monitor / control-plane coordinator.
 */

static bh_cap_t g_coremgr_endpoint = 0;
static bool g_shutdown_requested = false;

void init_coremgr(void) {
    coremgr_registry_init();

    // Register current core as a stub topology init
    coremgr_register_core(0, 0); // Bootstrap core
    coremgr_set_core_state(0, CORE_STATE_DISCOVERED);
    coremgr_set_core_state(0, CORE_STATE_STARTING);
    coremgr_set_core_state(0, CORE_STATE_ONLINE);

    // TODO: Obtain a valid IPC endpoint
    // bh_alloc_ex or similar endpoint creation is not directly mapped without ipc_endpoint_create,
    // which requires capability APIs. We mock endpoint creation for the deterministic testable loop.
    g_coremgr_endpoint = 1; // Dummy endpoint handle
}

// Deterministic testable event processing
int coremgr_process_event(void) {
    if (g_shutdown_requested) {
        return -1;
    }

    // In a real system, we would block on bh_ipc_recv.
    // For now, this is a stub that returns 0 (no event) to satisfy the deterministic loop.
    return 0;
}

#ifndef BHARAT_SERVICE_HOST_TEST
int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    init_coremgr();

    // Main event loop
    while (!g_shutdown_requested) {
        if (coremgr_process_event() < 0) {
            break;
        }

        // Wait or yield
        // We use bh_sleep_until or similar to avoid infinite CPU-spinning.
        // For the stub without full event primitive, we break after initialization if not in test to avoid hang in demo.
        break;
    }

    return 0;
}
#endif
