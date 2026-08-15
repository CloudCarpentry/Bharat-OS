#include "arch/arch_cpu_caps.h"
#include "hal/hal_memops.h"

void *hal_memcpy_gpr_bulk(void *, const void *, size_t);
void *hal_memset_gpr_bulk(void *, int, size_t);
void *hal_memmove_gpr_bulk(void *, const void *, size_t);
int hal_memcmp_gpr_bulk(const void *, const void *, size_t);

void arch_memops_register(size_t cpu_id, const arch_cpu_caps_record_t *caps) {
    (void)caps;
    const hal_memops_backend_t backend = {
        .copy = hal_memcpy_gpr_bulk,
        .set = hal_memset_gpr_bulk,
        .move = hal_memmove_gpr_bulk,
        .compare = hal_memcmp_gpr_bulk,
        .context_flags = BH_MEMCTX_F_DEFAULT | BH_MEMCTX_F_MAY_SLEEP |
                         BH_MEMCTX_F_NO_SIMD | BH_MEMCTX_F_NO_DMA |
                         BH_MEMCTX_F_NO_FAULT,
        .implementation_flags = HAL_MEMOPS_IMPL_F_GPR_ONLY,
    };
    (void)hal_memops_register(cpu_id, &backend);
}
