#ifndef BHARAT_HAL_CPU_LOCAL_H
#define BHARAT_HAL_CPU_LOCAL_H

// Architecture-neutral contract for CPU local pointer
struct cpu_local;
extern struct cpu_local *arch_cpu_local_ptr(void);

// Deprecated: use this_cpu() directly.
static inline struct cpu_local *hal_cpu_local_ptr(void) {
    return arch_cpu_local_ptr();
}

#endif // BHARAT_HAL_CPU_LOCAL_H
