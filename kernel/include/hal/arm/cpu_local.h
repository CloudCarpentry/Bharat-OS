#ifndef BHARAT_HAL_ARM_CPU_LOCAL_H
#define BHARAT_HAL_ARM_CPU_LOCAL_H

static inline cpu_local_t *hal_cpu_local_ptr(void) {
    cpu_local_t *cl;
    // For 32-bit ARM (ARMv7), typically use CP15 c13 thread id register (TPIDRPRW)
    // Here we use a fallback if not explicitly defined.
    extern uint32_t hal_get_cpu_id(void);
    cl = &g_cpu_locals[hal_get_cpu_id()];
    return cl;
}

#endif // BHARAT_HAL_ARM_CPU_LOCAL_H
