#include <bharat/cpu_local.h>

void arch_cpu_local_set(cpu_local_t *cl) {
    __asm__ volatile ("msr tpidr_el1, %0" :: "r"(cl));
}

cpu_local_t *arch_cpu_local_ptr(void) {
    cpu_local_t *cl;
    __asm__ volatile ("mrs %0, tpidr_el1" : "=r"(cl));
    return cl;
}
