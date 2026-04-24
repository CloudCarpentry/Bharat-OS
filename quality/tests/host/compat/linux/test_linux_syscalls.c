#include <stdio.h>
#include <assert.h>
#include "../../../../core/personalities/compat/linux/linux_compat.h"

// Mock for bh_translation_event_record
void bh_translation_event_record(int event) {
    (void)event;
}

int main() {
    printf("Running test_linux_syscalls...\n");

    subsys_instance_t env;
    env.type = SUBSYS_TYPE_LINUX;
    env.is_running = 1;
    env.memory_limit_mb = 1024;
    env.cpu_core_allocation_mask = 1;

    assert(linux_subsys_init(&env) == 0);

    // Test getpid (39)
    long ret = linux_syscall_handler(39, 0, 0, 0, 0, 0, 0);
    assert(ret == 1);

    // Test open (2)
    long fd = linux_syscall_handler(2, 0, 0, 0, 0, 0, 0);
    assert(fd >= 3);

    // Test read (0) on console
    ret = linux_syscall_handler(0, 0, 0, 0, 0, 0, 0);
    assert(ret == -38); // Console fallback

    // Test write (1) on console
    ret = linux_syscall_handler(1, 1, 0, 10, 0, 0, 0);
    assert(ret == 10);

    // Test close (3)
    ret = linux_syscall_handler(3, fd, 0, 0, 0, 0, 0);
    assert(ret == 0);

    printf("test_linux_syscalls passed!\n");
    return 0;
}
