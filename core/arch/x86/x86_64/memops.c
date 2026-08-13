#include "arch/arch_cpu_caps.h"
#include "hal/hal_memops.h"

void *hal_memcpy_fast_string(void *, const void *, size_t);
void *hal_memset_fast_string(void *, int, size_t);

void arch_memops_register(size_t cpu_id, const arch_cpu_caps_record_t *caps) {
    if (!arch_cpu_caps_test(&caps->usable, ARCH_CPU_FEAT_X86_ERMS)) return;
    const hal_memops_backend_t backend = {
        .copy = hal_memcpy_fast_string,
        .set = hal_memset_fast_string,
        .move = hal_memmove_scalar,
    };
    (void)hal_memops_register(cpu_id, &backend);
}
