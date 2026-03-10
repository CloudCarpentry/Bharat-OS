#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../subsys/windows/win_compat.h"

int main(void) {
    printf("Running Windows Subsystem integration tests...\n");

    subsys_instance_t win_env = {
        .type = SUBSYS_TYPE_WINDOWS,
        .is_running = 1
    };

    assert(winnt_subsys_init(&win_env) == 0);
    assert(win_env.memory_limit_mb == 2048U); // Verifies logic inside real code

    // Verify rejection of incorrect subsystem types
    subsys_instance_t linux_env = {
        .type = SUBSYS_TYPE_LINUX,
        .is_running = 1
    };
    assert(winnt_subsys_init(&linux_env) == -2);

    // Verify NULL environment handling
    assert(winnt_subsys_init(NULL) == -1);

    printf("Windows Subsystem integration tests passed successfully.\n");
    return 0;
}
