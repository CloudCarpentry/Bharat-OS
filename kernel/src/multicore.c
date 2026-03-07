#include "multicore.h"

#if defined(__riscv)
#include "boot/riscv/sbi.h"
#endif

int multicore_boot_secondary_cores(uint32_t core_count) {
    if (core_count <= 1U) {
        return 0;
    }

#if defined(__riscv)
    unsigned long hart_mask = 0UL;
    for (uint32_t hart = 1U; hart < core_count; ++hart) {
        hart_mask |= (1UL << hart);
    }

    if (hart_mask != 0UL) {
        sbi_send_ipi(&hart_mask);
    }
#else
    (void)core_count;
#endif

    return 0;
}
