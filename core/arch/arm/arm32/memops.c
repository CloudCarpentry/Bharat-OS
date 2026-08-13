#include "arch/arch_cpu_caps.h"
#include "hal/hal_memops.h"

void arch_memops_register(size_t cpu_id, const arch_cpu_caps_record_t *caps) {
    (void)cpu_id;
    (void)caps;
}
