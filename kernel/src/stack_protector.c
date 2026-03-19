#include "kernel.h"
#include <stdint.h>

uintptr_t __stack_chk_guard = 0x6A09E667F3BCC909ULL;

void __stack_chk_fail(void) {
    kernel_panic("stack protector check failed");
}
