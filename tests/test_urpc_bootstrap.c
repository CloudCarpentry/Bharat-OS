#include <stdio.h>
#include <assert.h>
#include "urpc/urpc_bootstrap.h"

int main(void) {
    printf("[TEST] Running URPC Bootstrap Tests...\n");

    int core_id = 2;

    assert(urpc_is_ready(core_id) == 0);

    // Bootstrap channel for core
    assert(urpc_bootstrap_core(core_id) == 0);

    // Verify it's ready
    urpc_mark_ready(core_id);
    assert(urpc_is_ready(core_id) == 1);

    // Test out of bounds
    assert(urpc_bootstrap_core(999) == -1);

    printf("[TEST] URPC Bootstrap Passed.\n");
    return 0;
}
