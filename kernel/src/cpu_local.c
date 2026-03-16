#include <bharat/cpu_local.h>
#include <stddef.h>

cpu_local_t g_cpu_locals[MAX_CPUS] __attribute__((aligned(64)));

void cpu_local_init(uint32_t cpu_id) {
    if (cpu_id >= MAX_CPUS) return;

    cpu_local_t *cl = &g_cpu_locals[cpu_id];
    cl->cpu_id = cpu_id;
    cl->current = NULL;
    cl->idle = NULL;
    
#if defined(__aarch64__)
    __asm__ volatile ("msr tpidr_el1, %0" :: "r"(cl));
#elif defined(__riscv)
    __asm__ volatile ("mv tp, %0" :: "r"(cl));
#elif defined(__x86_64__)
    // Usually written to MSR_GS_BASE by HAL
    // We defer to HAL or fallback array lookup.
#endif
}
