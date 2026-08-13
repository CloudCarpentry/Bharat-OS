#include "arch/arch_cpu_caps.h"
#include "hal/hal_memops.h"

void *hal_memcpy_fast_string(void *, const void *, size_t);
void *hal_memset_fast_string(void *, int, size_t);
void *hal_memmove_fast_string(void *, const void *, size_t);
int hal_memcmp_x86_gpr(const void *, const void *, size_t);
void *hal_memcpy_x86_gpr(void *, const void *, size_t);
void *hal_memset_x86_gpr(void *, int, size_t);

void arch_memops_register(size_t cpu_id, const arch_cpu_caps_record_t *caps) {
    const bool erms = arch_cpu_caps_test(&caps->usable, ARCH_CPU_FEAT_X86_ERMS);
    const hal_memops_backend_t backend = {
        .copy = erms ? hal_memcpy_fast_string : hal_memcpy_x86_gpr,
        .set = erms ? hal_memset_fast_string : hal_memset_x86_gpr,
        .move = hal_memmove_fast_string,
        .compare = hal_memcmp_x86_gpr,
        .context_flags = BH_MEMCTX_F_DEFAULT | BH_MEMCTX_F_MAY_SLEEP |
                         BH_MEMCTX_F_NO_SIMD | BH_MEMCTX_F_NO_DMA |
                         BH_MEMCTX_F_NO_FAULT,
        .implementation_flags = HAL_MEMOPS_IMPL_F_GPR_ONLY |
                                (erms ? HAL_MEMOPS_IMPL_F_FAST_STRING : 0u),
    };
    (void)hal_memops_register(cpu_id, &backend);
}
