#include "multicore.h"
#include "bharat_config.h"
#include "core/multikernel.h"

#if defined(__riscv)
#include "../../arch/riscv/boot/sbi.h"
#endif

#define KERNEL_STACK_SIZE 16384 // 16 KiB
uint8_t g_per_core_stacks[MAX_SUPPORTED_CORES][KERNEL_STACK_SIZE] __attribute__((aligned(16)));

static uint32_t g_system_core_count = 1U;

// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
#include "arch/arch_caps.h"

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
// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
#include "hal/hal.h"

void smp_init(void) {
    if (!arch_has_cap(ARCH_CAP_SMP)) {
        return;
    }

    uint32_t core_id = hal_cpu_get_id();

    // Primary boot sequence setup
    if (core_id == 0U) {
        // Initialize URPC channels matrix for all cores.
        // Using a default ring size of 1024 for example.
        mk_init_per_core_channels(g_system_core_count, 1024U);
    }

    // Each core establishes its specific channels
    for (uint32_t i = 0U; i < g_system_core_count; ++i) {
        if (i != core_id) {
            mk_channel_t chan;
            mk_establish_channel(i, &chan);
        }
    }

    // Further AP local state initialization
    // For phase 1, APs just need to enable interrupts and enter their scheduler tick
    hal_cpu_enable_interrupts();
}
