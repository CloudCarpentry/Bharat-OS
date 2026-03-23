#include <stdio.h>
#include <stdlib.h>
void panic(const char *msg) {
    printf("PANIC: %s\n", msg);
    abort();
}

__attribute__((weak)) void kernel_panic(const char *msg) {
    printf("KERNEL_PANIC: %s\n", msg);
    abort();
}

const char* console_current_phase(void) {
    return "MOCK_PANIC_PHASE";
}

__attribute__((weak)) int aspace_destroy(void *aspace) {
    (void)aspace;
    return 0;
}

__attribute__((weak)) void vm_debug_validate_active_tracking(void) {}
