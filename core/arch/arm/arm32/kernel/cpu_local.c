#include <bharat/cpu_local.h>

void arch_cpu_local_set(cpu_local_t *cl) {
    // ARM32 CP15 register for thread ID
    __asm__ volatile ("mcr p15, 0, %0, c13, c0, 3" :: "r"(cl));
}
