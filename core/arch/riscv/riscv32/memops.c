#include "arch/arch_cpu_caps.h"
#include "hal/hal_memops.h"

void arch_memops_register(size_t cpu_id, const arch_cpu_caps_record_t *caps) {
    (void)caps;
    extern void *hal_memcpy_rv32_gpr(void *, const void *, size_t);
    extern void *hal_memset_rv32_gpr(void *, int, size_t);
    extern void *hal_memmove_rv32_gpr(void *, const void *, size_t);
    extern int hal_memcmp_rv32_gpr(const void *, const void *, size_t);
    const hal_memops_backend_t backend = {
        .copy = hal_memcpy_rv32_gpr,
        .set = hal_memset_rv32_gpr,
        .move = hal_memmove_rv32_gpr,
        .compare = hal_memcmp_rv32_gpr,
        .context_flags = BH_MEMCTX_F_DEFAULT | BH_MEMCTX_F_MAY_SLEEP |
                         BH_MEMCTX_F_NO_SIMD | BH_MEMCTX_F_NO_DMA |
                         BH_MEMCTX_F_NO_FAULT,
        .implementation_flags = HAL_MEMOPS_IMPL_F_GPR_ONLY,
    };
    (void)hal_memops_register(cpu_id, &backend);
}
