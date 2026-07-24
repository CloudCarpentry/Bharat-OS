#include <bharat/cpu_local.h>
#include <stddef.h>

extern void arch_cpu_local_set(cpu_local_t *cl);

cpu_local_t g_cpu_locals[MAX_CPUS] __attribute__((aligned(64)));

void cpu_local_init(uint32_t cpu_id) {
    if (cpu_id >= MAX_CPUS) return;

    cpu_local_t *cl = &g_cpu_locals[cpu_id];
    cl->cpu_id = cpu_id;
    cl->current = NULL;
    cl->idle = NULL;
    cl->current_as_id = 0;
    cl->current_as = NULL;

    arch_cpu_local_set(cl);
}
