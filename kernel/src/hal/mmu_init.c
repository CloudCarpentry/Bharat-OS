#include "../../include/hal/mmu_ops.h"
#include <stddef.h>

mmu_ops_t *active_mmu = NULL;

// Forward declarations for arch-specific ops
extern mmu_ops_t x86_64_mmu_ops;
extern mmu_ops_t arm64_mmu_ops;
extern mmu_ops_t riscv64_mmu_ops;
extern mmu_ops_t arm32_mpu_ops;

// Forward declarations for runtime detection
extern void x86_mmu_detect(mmu_ops_t *ops);
extern void arm64_mmu_detect(mmu_ops_t *ops);
extern void riscv_mmu_detect(mmu_ops_t *ops);
extern void arm32_mmu_detect(mmu_ops_t *ops);

void arch_mmu_init(void) {
#if defined(__x86_64__)
    active_mmu = &x86_64_mmu_ops;
    x86_mmu_detect(active_mmu);

#elif defined(__aarch64__)
    active_mmu = &arm64_mmu_ops;
    arm64_mmu_detect(active_mmu);

#elif defined(__riscv) && __riscv_xlen == 64
    active_mmu = &riscv64_mmu_ops;
    riscv_mmu_detect(active_mmu);

#elif defined(__arm__)
    active_mmu = &arm32_mpu_ops;
    arm32_mmu_detect(active_mmu);

#else
    // Fallback or unsupported architecture
    active_mmu = NULL;
#endif
}
