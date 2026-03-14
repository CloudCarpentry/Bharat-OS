#include "../../include/hal/mmu_ops.h"
#include <stddef.h>

mmu_ops_t *active_mmu = NULL;

#include "../../include/profile.h"

// Forward declarations for arch-specific ops
extern mmu_ops_t x86_64_mmu_ops;
extern mmu_ops_t arm64_mmu_ops;
extern mmu_ops_t riscv64_mmu_ops;

// Forward declarations for runtime detection
extern void x86_mmu_detect(mmu_ops_t *ops);
extern void arm64_mmu_detect(mmu_ops_t *ops);
extern void riscv_mmu_detect(mmu_ops_t *ops);

// Forward declarations for IOMMU detection
extern void x86_iommu_detect(void);
extern void arm64_iommu_detect(void);
extern void riscv_iommu_detect(void);

// Initialize based on memory model profile
void arch_mmu_init(void) {
    MemoryModel model = get_memory_model();

    if (model == MEM_MODEL_MMU) {
#if defined(__x86_64__)
        active_mmu = &x86_64_mmu_ops;
        x86_mmu_detect(active_mmu);
        x86_iommu_detect();

#elif defined(__aarch64__)
        active_mmu = &arm64_mmu_ops;
        arm64_mmu_detect(active_mmu);
        arm64_iommu_detect();

#elif defined(__riscv) && __riscv_xlen == 64
        active_mmu = &riscv64_mmu_ops;
        riscv_mmu_detect(active_mmu);
        riscv_iommu_detect();
#else
        active_mmu = NULL;
#endif
    } else {
        // MPU or FLAT profile: Do not provide MMU ops.
        // MPU has its own specific region management APIs not exposed via mmu_ops_t.
        active_mmu = NULL;
    }
}
