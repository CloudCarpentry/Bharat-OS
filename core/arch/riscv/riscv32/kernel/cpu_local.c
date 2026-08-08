#include <bharat/cpu_local.h>

void arch_cpu_local_set(cpu_local_t *cl) {
    __asm__ volatile ("mv tp, %0" : : "r" (cl));
}

cpu_local_t *arch_cpu_local_ptr(void) {
    cpu_local_t *cl;
    __asm__ volatile ("mv %0, tp" : "=r" (cl));
    return cl;
}
