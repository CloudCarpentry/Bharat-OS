#include "../../../include/arch/cpu_relax.h"

void arch_cpu_relax(void) {
    __asm__ volatile("pause" ::: "memory");
}
