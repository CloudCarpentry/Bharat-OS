#include <bharat/cpu_local.h>

void arch_cpu_local_set(cpu_local_t *cl) {
    __asm__ volatile ("msr tpidr_el1, %0" :: "r"(cl));
}
