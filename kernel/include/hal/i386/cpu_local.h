#ifndef BHARAT_HAL_I386_CPU_LOCAL_H
#define BHARAT_HAL_I386_CPU_LOCAL_H

static inline cpu_local_t *hal_cpu_local_ptr(void) {
    cpu_local_t *cl;
    // For 32-bit x86, we'd typically use FS base, but let's fall back
    // to getting the CPU ID if there's no fast path established yet.
    extern uint32_t hal_get_cpu_id(void);
    cl = &g_cpu_locals[hal_get_cpu_id()];
    return cl;
}

#endif // BHARAT_HAL_I386_CPU_LOCAL_H
