#ifndef BHARAT_HAL_CPU_LOCAL_H
#define BHARAT_HAL_CPU_LOCAL_H

// Dispatch to the correct architecture-specific CPU local header

#if defined(__x86_64__)
#include <hal/x86_64/cpu_local.h>
#elif defined(__aarch64__)
#include <hal/aarch64/cpu_local.h>
#elif defined(__riscv) && __riscv_xlen == 64
#include <hal/riscv64/cpu_local.h>
#elif defined(__riscv) && __riscv_xlen == 32
#include <hal/riscv32/cpu_local.h>
#elif defined(__arm__)
#include <hal/arm/cpu_local.h>
#elif defined(__i386__)
#include <hal/i386/cpu_local.h>
#else

// Fallback for unknown architectures or test environments
static inline cpu_local_t *hal_cpu_local_ptr(void) {
    extern uint32_t hal_get_cpu_id(void);
    return &g_cpu_locals[hal_get_cpu_id()];
}

#endif

#endif // BHARAT_HAL_CPU_LOCAL_H
