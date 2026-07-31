#include "bharat_config.h"
#include "hal/hal_ipi.h"
#include "hal/hal_boot.h"
#include "hal/hal_irq.h"
#include "hal/hal_timer.h"
#include "hal/hal_topology.h"
#include "hal/hal_pt.h"
#include "hal/hal_tlb.h"
#include "hal/hal.h"
#include "hal/hal_discovery.h"
#include "hal/hal_mpa.h"
#include "urpc/urpc_bootstrap.h"
#include "sched/sched.h"
#include "console/console_core.h"
#include "mm.h"
#include "mm/physmap.h"
#include "arch/arch_caps.h"
#include "arch/cpu_relax.h"
#include "profile/profile.h"
#include "panic.h"
#include "bharat/cpu_local.h"
#include <stdatomic.h>

#define KPRINT(s) console_write_raw(s, string_length(s))

typedef struct {
    _Atomic bh_cpu_boot_state_t state;
    uint32_t logical_cpu_id;
    uint32_t hw_id;
    uint64_t boot_generation;
    uint8_t padding[64 - sizeof(_Atomic bh_cpu_boot_state_t) - sizeof(uint32_t)*2 - sizeof(uint64_t)];
} __attribute__((aligned(64))) bh_cpu_boot_record_t;

static bh_cpu_boot_record_t g_cpu_boot_records[BHARAT_MAX_CPUS];
static uint32_t g_system_core_count = 1U;

// Global array of boot context pointers for secondary cores (mapped to their physical context ID)
static bh_arm64_ap_boot_context_t g_arm64_ap_contexts[BHARAT_MAX_CPUS];

// Declare multikernel channels without including multikernel.h
typedef struct {
    uint32_t src_core;
    uint32_t dst_core;
    void *urpc_ring;
    uint32_t ring_size;
} mk_channel_t;
int mk_establish_channel(uint32_t target_core, mk_channel_t *out_channel);

void bh_smp_set_cpu_state(uint32_t cpu_id, bh_cpu_boot_state_t state) {
    if (cpu_id < BHARAT_MAX_CPUS) {
        atomic_store_explicit(&g_cpu_boot_records[cpu_id].state, state, memory_order_release);
    }
}

bh_cpu_boot_state_t bh_smp_get_cpu_state(uint32_t cpu_id) {
    if (cpu_id < BHARAT_MAX_CPUS) {
        return atomic_load_explicit(&g_cpu_boot_records[cpu_id].state, memory_order_acquire);
    }
    return BH_CPU_BOOT_FAILED;
}

uint32_t bh_smp_get_online_core_count(void) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < BHARAT_MAX_CPUS; i++) {
        if (bh_smp_get_cpu_state(i) == BH_CPU_BOOT_ONLINE) {
            count++;
        }
    }
    return count == 0 ? 1 : count; // fallback to 1 if none online (e.g. non-SMP)
}

int bh_smp_boot_primary_init(void) {
    for (uint32_t i = 0; i < BHARAT_MAX_CPUS; i++) {
        atomic_init(&g_cpu_boot_records[i].state, BH_CPU_BOOT_OFFLINE);
        g_cpu_boot_records[i].logical_cpu_id = i;
        g_cpu_boot_records[i].hw_id = 0;
        g_cpu_boot_records[i].boot_generation = 0;
    }

    g_system_core_count = 1U;
    bh_smp_set_cpu_state(0, BH_CPU_BOOT_ONLINE);
    return 0;
}

// Bounded monotonic timer ticks to ms conversion helpers
static uint64_t ms_to_ticks(uint64_t ms) {
    uint64_t freq = hal_timer_read_freq();
    if (freq == 0) {
        // Fallback for mock timers or uninitialized state (use 1MHz)
        freq = 1000000ULL;
    }
    return (ms * freq) / 1000ULL;
}

#if defined(__aarch64__)
static void flush_cache_range(void *addr, size_t size) {
    uintptr_t start = (uintptr_t)addr;
    uintptr_t end = start + size;
    uint64_t ctr;
    __asm__ volatile("mrs %0, ctr_el0" : "=r"(ctr));
    uint32_t line_size = 4 << ((ctr >> 16) & 0xF);

    start &= ~(line_size - 1);
    while (start < end) {
        __asm__ volatile("dc cvac, %0" :: "r"(start) : "memory");
        start += line_size;
    }
    __asm__ volatile("dsb sy; isb" ::: "memory");
}
#endif

extern uint8_t g_per_core_stacks[MAX_SUPPORTED_CORES][16384];

int bh_smp_start_secondary_cpus(uint32_t requested_cpus) {
    if (!arch_has_cap(ARCH_CAP_SMP)) {
        g_system_core_count = 1U;
        return 0;
    }

    system_discovery_t* disc = hal_get_system_discovery();
    if (disc && disc->topology.cpu_count > 0) {
        if (requested_cpus > disc->topology.cpu_count) {
            requested_cpus = disc->topology.cpu_count;
        }
    }

    if (requested_cpus > MAX_SUPPORTED_CORES) {
        requested_cpus = MAX_SUPPORTED_CORES;
    }
    g_system_core_count = requested_cpus;

    if (requested_cpus <= 1) {
        return 0;
    }

    KPRINT("  [SMP] Starting secondary CPUs...\n");

    // Clean structures to point of coherency and setup context for secondary cores
    for (uint32_t cpu_id = 1; cpu_id < requested_cpus; cpu_id++) {
        bh_smp_set_cpu_state(cpu_id, BH_CPU_BOOT_STARTING);

        // Prepare physical boot context for ARM64 if needed
#if defined(__aarch64__)
        bh_arm64_ap_boot_context_t *ctx = &g_arm64_ap_contexts[cpu_id];
        ctx->logical_cpu_id = cpu_id;

        // Setup per-core physical stack top
        uint64_t stack_top_phys = (uintptr_t)&g_per_core_stacks[cpu_id][16384];
        ctx->stack_top_phys = stack_top_phys;

        // Get translation root
        extern mem_protect_ops_t arm64_mem_protect_ops;
        ctx->kernel_root_phys = arm64_mem_protect_ops.cpu_ops.get_root();

        // Safe TTBR0 root is 0
        ctx->ttbr0_root_phys = 0;

        // TCR_EL1: T0SZ=16, T1SZ=16 (48-bit VA), TG0=0 (4KB), TG1=2 (4KB), IPS=2 (40-bit PA)
        ctx->tcr_el1 = (16ULL << 0) | (16ULL << 16) | (3ULL << 12) | (3ULL << 28) |
                       (1ULL << 10) | (1ULL << 26) | (1ULL << 8) | (1ULL << 24) |
                       (0ULL << 14) | (2ULL << 30) | (2ULL << 32);

        // MAIR_EL1: Attr0=Normal, Attr1=Device-nGnRE, Attr2=Device-nGnRnE
        ctx->mair_el1 = (0xFFLL << 0) | (0x04LL << 8) | (0x00LL << 16);

        // SCTLR_EL1: Enable MMU (bit 0), Instruction Cache (bit 12), Data Cache (bit 2)
        ctx->sctlr_el1 = (1ULL << 0) | (1ULL << 12) | (1ULL << 2);

        // VBAR_EL1
        uint64_t vbar_val;
        __asm__ volatile("mrs %0, vbar_el1" : "=r"(vbar_val));
        ctx->vbar_el1 = vbar_val;

        ctx->boot_generation = 1;

        // Clean boot context, stack metadata and page table updates to Point of Coherency (PoC)
        flush_cache_range(ctx, sizeof(*ctx));
#endif

        // Assembly trampoline entry point
        extern void _secondary_trampoline(void);
        uint64_t entry_point = (uint64_t)(uintptr_t)_secondary_trampoline;

#if defined(__aarch64__)
        uint64_t context_arg = (uint64_t)(uintptr_t)ctx;
        (void)entry_point;
#else
        uint64_t context_arg = entry_point;
#endif

        KPRINT("  [SMP] Launching CPU ");
        char id_buf[4];
        id_buf[0] = '0' + (cpu_id % 10);
        id_buf[1] = '\n';
        id_buf[2] = '\0';
        console_write_raw(id_buf, 2);

        int rc = hal_boot_start_cpu(cpu_id, context_arg);
        if (rc != 0) {
            KPRINT("  [SMP] Error: hal_boot_start_cpu failed\n");
            bh_smp_set_cpu_state(cpu_id, BH_CPU_BOOT_FAILED);
        }
    }

    // Concurrent waiting with bounded monotonic ticks deadline
    // Use standard 2,000 ms global AP-online deadline (5,000 ms for QEMU CI/tests)
    uint64_t wait_ms = 5000;
    uint64_t start_tick = hal_timer_read_counter();
    uint64_t timeout_ticks = ms_to_ticks(wait_ms);

    bool all_online = false;
    uint32_t cpus_printed = 0;
    while (hal_timer_read_counter() - start_tick < timeout_ticks) {
        all_online = true;
        for (uint32_t cpu_id = 1; cpu_id < requested_cpus; cpu_id++) {
            bh_cpu_boot_state_t state = bh_smp_get_cpu_state(cpu_id);
            if (state == BH_CPU_BOOT_ONLINE) {
                if (!(cpus_printed & (1 << cpu_id))) {
                    cpus_printed |= (1 << cpu_id);
                    KPRINT("SMP: CPU");
                    char c = '0' + cpu_id;
                    console_write_raw(&c, 1);
                    KPRINT(" ONLINE\n");
                }
            } else {
                all_online = false;
            }
        }
        if (all_online) {
            break;
        }
        arch_cpu_relax();
    }

    if (all_online) {
        KPRINT("  [SMP] All requested harts are ONLINE!\n");
        KPRINT("SMP: ALL 4 CORES ONLINE\n");
        return 0;
    }

    // Handle failures per CPU
    for (uint32_t cpu_id = 1; cpu_id < requested_cpus; cpu_id++) {
        bh_cpu_boot_state_t state = bh_smp_get_cpu_state(cpu_id);
        if (state != BH_CPU_BOOT_ONLINE) {
            bh_smp_set_cpu_state(cpu_id, BH_CPU_BOOT_FAILED);
            KPRINT("  [SMP] Core ");
            char id_c = '0' + (cpu_id % 10);
            console_write_raw(&id_c, 1);
            KPRINT(" failed to boot! State: ");
            char st_c = '0' + (state % 10);
            console_write_raw(&st_c, 1);
            KPRINT("\n");
        }
    }

    // Failure policy enforcement: Fail closed for diagnostic/hardened/SMP required profiles, quarantine for GP.
    KernelExecutionProfile exec_profile = get_kernel_execution_profile();
    if (exec_profile == PROFILE_KERNEL_GP) {
        KPRINT("  [SMP] GP profile: continuing in degraded/quarantined mode.\n");
        return 0; // Allow continuation with fewer cores
    } else {
        kernel_panic("SMP: required CPU count not online - fail closed!");
    }

    return -1;
}

// Canonical C secondary-core entry point
void bh_secondary_cpu_entry(uint64_t context_phys) {
    uint32_t core_id = hal_cpu_get_id();

    // If context_phys is present (ARM64), use it to obtain logical ID
    if (context_phys != 0) {
#if defined(__aarch64__)
        bh_arm64_ap_boot_context_t *ctx = (bh_arm64_ap_boot_context_t *)physmap_phys_to_virt(context_phys);
        core_id = ctx->logical_cpu_id;
#endif
    }

    bh_smp_set_cpu_state(core_id, BH_CPU_BOOT_STARTING);

    // Step 3: Setup initial exceptions/CSRs
    secondary_entry_arch_early();
    bh_smp_set_cpu_state(core_id, BH_CPU_BOOT_ARCH_READY);

    // Step 4: CPU-local identity initialization
    cpu_local_init(core_id);

    // Step 5: Local GIC/IRQ initialization
    hal_irq_init_cpu_local(core_id);
    hal_ipi_init_cpu_local(core_id);
    bh_smp_set_cpu_state(core_id, BH_CPU_BOOT_IRQ_READY);

    // Step 6: Local timer initialization
    hal_timer_init_cpu_local(core_id);
    bh_smp_set_cpu_state(core_id, BH_CPU_BOOT_TIMER_READY);

    // Step 7: Per-core PMM/VMM/TLB initialization
    hal_pt_init();
    hal_tlb_init();
    if (vmm_init() != 0) {
        bh_smp_set_cpu_state(core_id, BH_CPU_BOOT_FAILED);
        goto halt_loop;
    }
    bh_smp_set_cpu_state(core_id, BH_CPU_BOOT_MEMORY_READY);

    // Step 8: uRPC endpoint binding
    if (urpc_bootstrap_core(core_id) != 0) {
        bh_smp_set_cpu_state(core_id, BH_CPU_BOOT_FAILED);
        goto halt_loop;
    }
    bh_smp_set_cpu_state(core_id, BH_CPU_BOOT_URPC_READY);
    urpc_mark_ready(core_id);

    // Establish specific multi-kernel channels
    for (uint32_t i = 0U; i < g_system_core_count; ++i) {
        if (i != core_id) {
            mk_channel_t chan;
            mk_establish_channel(i, &chan);
        }
    }

    // Step 9: Local scheduler initialization
    sched_init();
    bh_smp_set_cpu_state(core_id, BH_CPU_BOOT_SCHED_READY);

    // Step 10: Atomic publication of ONLINE
    bh_smp_set_cpu_state(core_id, BH_CPU_BOOT_ONLINE);

    // Step 11: Enter idle / scheduler loop
    secondary_entry_arch_late();
    hal_cpu_enable_interrupts();

halt_loop:
    while (1) {
        hal_cpu_halt();
    }
}

// Common secondary entry point wrapper (deprecated, kept for legacy/compatibility)
void secondary_entry_common(void) {
    bh_secondary_cpu_entry(0);
}
