#include "../../kernel/include/arch/cpu_relax.h"

void arch_cpu_relax(void) {
    __asm__ volatile("yield" ::: "memory");
}
