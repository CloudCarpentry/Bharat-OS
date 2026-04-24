#include <bharat/cpu_local.h>

void arch_cpu_local_set(cpu_local_t *cl) {
    // Usually written to MSR_GS_BASE by HAL
    // We defer to HAL or fallback array lookup.
    (void)cl;
}
