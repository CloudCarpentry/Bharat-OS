#ifndef BHARAT_HAL_X86_64_CPU_LOCAL_H
#define BHARAT_HAL_X86_64_CPU_LOCAL_H

static inline cpu_local_t *hal_cpu_local_ptr(void) {
    cpu_local_t *cl;
    // Assume we've written the address into GSBASE
    __asm__ volatile ("movq %%gs:0, %0" : "=r"(cl));
    return cl;
}

#endif // BHARAT_HAL_X86_64_CPU_LOCAL_H
