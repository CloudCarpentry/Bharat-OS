#include "multicore.h"
#include "bharat_config.h"
#include "core/multikernel.h"
#include "arch/arch_caps.h"
#include "hal/hal.h"
#include "hal/hal_pt.h"
#include "hal/hal_tlb.h"
#include "kernel.h"
#include "mm.h"
#include "sched/sched.h"

#if defined(__riscv)
#include "../../arch/riscv/boot/sbi.h"
#endif

#define KERNEL_STACK_SIZE 16384 // 16 KiB
uint8_t g_per_core_stacks[MAX_SUPPORTED_CORES][KERNEL_STACK_SIZE] __attribute__((aligned(16)));

static uint32_t g_system_core_count = 1U;

int multicore_boot_secondary_cores(uint32_t core_count) {
    if (!arch_has_cap(ARCH_CAP_SMP)) {
        g_system_core_count = 1U;
        return 0;
    }

    if (core_count > MAX_SUPPORTED_CORES) {
        core_count = MAX_SUPPORTED_CORES;
    }
    g_system_core_count = core_count;
    if (core_count <= 1U) {
        return 0;
    }

#if defined(__riscv)
    unsigned long hart_mask = 0UL;
    for (uint32_t hart = 1U; hart < core_count; ++hart) {
        hart_mask |= (1UL << hart);
    }

    if (hart_mask != 0UL) {
        sbi_send_ipi(hart_mask, 0UL);
    }
#else
    (void)core_count;
#endif

    return 0;
}
