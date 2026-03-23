#ifndef BHARAT_HAL_RISCV64_CPU_LOCAL_H
#define BHARAT_HAL_RISCV64_CPU_LOCAL_H

static inline cpu_local_t *hal_cpu_local_ptr(void) {
    cpu_local_t *cl;
    __asm__ volatile ("mv %0, tp" : "=r"(cl));
    return cl;
}

#endif // BHARAT_HAL_RISCV64_CPU_LOCAL_H
