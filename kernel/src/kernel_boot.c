#include "hal/hal_irq.h"
#include "hal/hal_timer.h"
#include "../staging/ai/ai_kernel_bridge.h"
#include "sched/ai_sched.h"
#include "sched/algo_matrix.h"
#include "core/multikernel.h"
#include "device.h"
#include "hal/hal.h"
#include "hal/hal_pt.h"
#include "hal/hal_tlb.h"
#include "hal/hal_discovery.h"
#include "ipc_async.h"
#include "kernel.h"
#include "console/console_core.h"
#include "mm.h"
#include "arch/arch_cpu_caps.h"
#include "mm_zswap.h"
#include "multicore.h"
#include "numa.h"
#include "power_thermal_perf.h"
#include "profile/profile.h"
#include "secure_boot.h"
#include "security/audit.h"
#include "security/credentials.h"
#include "security/isolation.h"
#include "security/policy.h"
#include "profile/subsystem_profile.h"
#include "trap.h"
#include "boot/boot_args.h"
#include "boot/boot_info.h"
#include "boot/boot_validate.h"
#include "boot/boot_security.h"
#include "display/boot_gui_init.h"
#include "tests/ktest.h"
#include <bharat/cpu_local.h>
#include "arch/arch_ext_state.h"
#include "arch/arch_cpu_caps.h"
#include "arch/arch_caps.h"
#include "boot/boot_mode.h"
#include "boot/boot_selftest.h"

#define KPRINT(s) console_write_raw(s, string_length(s))

static void print_boot_diagnostics(const boot_info_t *boot) {
    if (!boot) return;

    KPRINT("  [BOOT] Normalized handoff contract established\n");

    boot_validation_report_t report;
    int vret = boot_validate_all((boot_info_t*)boot, &report);
    if (vret != BOOT_OK) {
        KPRINT("  [BOOT] Validation failed: ");
        if (report.message) KPRINT(report.message);
        KPRINT("\n");
        if (report.is_fatal) {
            kernel_panic("Fatal boot handoff validation error");
        }
    }

    if (boot->is_degraded) {
        KPRINT("  [BOOT] Running in degraded mode (some handoff info missing/invalid)\n");
    }

    bharat_boot_mode_t mode;
    boot_mode_resolve(boot, &mode);
    ((boot_info_t*)boot)->selected_mode = (boot_mode_t)mode; // Cast away const to set policy state
}

// External linker symbols for the subsystem registration framework
extern const subsystem_descriptor_t __start_bharat_services;
extern const subsystem_descriptor_t __stop_bharat_services;

void init_subsystems(void) {
    const subsystem_descriptor_t* current = &__start_bharat_services;

    // Default boot profile
    uint32_t active_profile_mask = 1;

    while (current < &__stop_bharat_services) {
        if ((current->profile_mask & active_profile_mask) != 0) {
            // Profile matched, start the subsystem
            if (current->init_fn) {
                current->init_fn();
            }
        }
        current++;
    }
}
void boot_common_early(const boot_info_t *boot) {


    hal_init();

    KPRINT("\n");
    KPRINT("  ____  _                          _          ____   ____  \n");
    KPRINT(" | __ )| |__   __ _ _ __ __ _ _| |_       / ___| / ___| \n");
    KPRINT(" |  _ \\| '_ \\ / _` | '__/ _` | '__/ _` |_____| |  _  \\___ \\ \n");
    KPRINT(" | |_) | | | | (_| | | | (_| | | | (_| |_____| |_| |  ___) |\n");
    KPRINT(" |____/|_| |_|\\__,_|_|  \\__,_|_|  \\__,_|      \\____| |____/ \n");
    KPRINT("\nBharat-OS\n");

    print_boot_diagnostics(boot);

    KPRINT("  [HAL] Initialising hardware on BSP...\n");
    hal_discovery_init(boot);
    KPRINT("  [HAL] Ready.\n");

    KPRINT("  [PROFILE] Applying hardware profile hooks...\n");
    profile_init();

    KernelExecutionProfile exec_profile = get_kernel_execution_profile();
    if (exec_profile == PROFILE_KERNEL_RT) {
        KPRINT("  [PROFILE] Kernel Execution Profile: RT (Real-Time)\n");
    } else if (exec_profile == PROFILE_KERNEL_MIX) {
        KPRINT("  [PROFILE] Kernel Execution Profile: MIX (Mixed RT/GP)\n");
    } else {
        KPRINT("  [PROFILE] Kernel Execution Profile: GP (General Purpose)\n");
    }


    // Print selected capabilities
    arch_caps_t arch_caps = arch_get_caps();
    if (arch_caps_test(arch_caps, ARCH_CAP_MMU_FULL)) {
        KPRINT("  [CAP] Protection Profile: MMU_FULL\n");
    } else if (arch_caps_test(arch_caps, ARCH_CAP_MMU_LITE)) {
        KPRINT("  [CAP] Protection Profile: MMU_LITE\n");
    } else if (arch_caps_test(arch_caps, ARCH_CAP_MPU_ONLY)) {
        KPRINT("  [CAP] Protection Profile: MPU_ONLY\n");
    } else {
        KPRINT("  [CAP] Protection Profile: UNKNOWN\n");
    }

    if (arch_caps_test(arch_caps, ARCH_CAP_SMP)) {
        KPRINT("  [CAP] SMP Enabled\n");
    } else {
        KPRINT("  [CAP] UP Only\n");
    }

    init_subsystems();

    boot_selftest_report_t report;
    boot_selftest_run_stage(BOOT_TEST_STAGE_EARLY, &report);
}

void boot_common_security(const boot_info_t *boot) {
    (void)boot;
    KPRINT("  [SEC] Running secure-boot verification...\n");
    if (bharat_secure_boot_verify_early() != 0) {
      kernel_panic("secure-boot verification failed");
    }
    (void)bharat_audit_init();
    (void)bharat_credentials_init();
    (void)bharat_isolation_init();
    (void)bharat_policy_init(BHARAT_POLICY_MODE_ENFORCING);
    (void)bharat_secure_boot_stage_hook(BHARAT_BOOT_STAGE_KERNEL,
                                        0xB4AA7001ULL);
    KPRINT("  [SEC] Secure-boot policy accepted.\n");

    KPRINT("  [ALGO] Initializing Capability Matrix...\n");
    algo_matrix_init();
    KPRINT("  [ALGO] Matrix Ready.\n");

    boot_selftest_report_t report;
    boot_selftest_run_stage(BOOT_TEST_STAGE_SECURITY, &report);
}

void boot_common_memory(const boot_info_t *boot) {
    KPRINT("  [MM]  Initializing PMM...\n");

    // Abstracted PMM initialization - using the normalized boot info
    if (mm_pmm_init(boot->magic, boot) != 0) {
      kernel_panic("PMM initialization failed");
    }
    KPRINT("BOOT: pmm initialized\n");

    KPRINT("  [VMM] Initializing VMM...\n");
    if (vmm_init() != 0) {
      kernel_panic("VMM initialization failed");
    }
    KPRINT("BOOT: vmm initialized\n");

    // Ensure hal_pt is initialized
    hal_pt_init();
    hal_tlb_init();

    // The rest of the setup is handled through the hal_pt interface
    KPRINT("  [VMM] Architecture MMU mappings configured.\n");

    const bharat_boot_policy_t *boot_policy = bharat_boot_active_policy();
    if (boot_policy->enable_zswap != 0U) {
      KPRINT("  [ZSWAP] Initializing Memory Compression...\n");
      if (zswap_init() != 0) {
        kernel_panic("ZSWAP initialization failed");
      }
      KPRINT("BOOT: zswap initialized\n");
    }

    boot_selftest_report_t report;
    boot_selftest_run_stage(BOOT_TEST_STAGE_MEMORY, &report);
}

static mk_channel_t g_scheduler_ai_channel;

static void kernel_ai_governor_init(void) {
  if (mk_establish_channel(0U, &g_scheduler_ai_channel) == 0) {
    KPRINT("  [AI]  Scheduler control channel ready.\n");
  } else {
    kernel_panic("failed to establish AI scheduler control channel");
  }
}

void boot_common_platform_services(const boot_info_t *boot) {
    (void)boot;
    const bharat_boot_policy_t *boot_policy = bharat_boot_active_policy();

    KPRINT("  [PTP] Initializing power/thermal/perf manager\n");
    if (ptp_init() != 0) {
      kernel_panic("power/thermal/perf init failed");
    }

    KPRINT("  [NUMA] Discovering topology\n");
    if (numa_discover_topology() != 0) {
      kernel_panic("numa topology discovery failed");
    }

    KPRINT("  [SMP] Booting secondary cores\n");
    if (mk_boot_secondary_cores(boot_policy->smp_target_cores) != 0) {
      kernel_panic("secondary core boot failed");
    }

    KPRINT("  [SMP] Initializing per-core URPC channels\n");
    if (mk_init_per_core_channels(boot_policy->smp_target_cores, 32U) != 0) {
      kernel_panic("per-core urpc channel init failed");
    }

    KPRINT("  [IRQ] Initializing interrupt controller\n");
    hal_irq_init_boot();

    KPRINT("  [TMR] Initializing timer source\n");
    hal_timer_init();

    KPRINT("  [DEV] Initializing device framework\n");
    if (device_framework_init() != 0 ||
        device_register_builtin_drivers() != 0) {
      kernel_panic("device framework initialization failed");
    }

    // Trigger test for DMA capability resolution dumping
    extern void test_device_dma_dump(void);
    test_device_dma_dump();

    arch_cpu_caps_init();
    arch_cpu_caps_system_finalize();
    hal_discovery_publish_cpu_caps();
    arch_ext_state_boot_init();

    sched_init();
    KPRINT("  [SCHED] Scheduler initialized.\n");

    KPRINT("  [AI] Calibrating hardware silicon metrics...\n");
    ai_sched_calibrate_silicon();
    KPRINT("  [AI] Calibration complete.\n");

    KPRINT("  [IPC] Initializing Async IPC subsystem...\n");
    ipc_async_init();
    KPRINT("  [IPC] Async IPC ready.\n");

    KPRINT("  [TRAP] Initializing syscall/trap gate...\n");
#include "personality/personality_hooks.h"
    extern const personality_ops_t default_personality_ops;
    personality_register_ops(&default_personality_ops);

    if (trap_init() != 0) {
      kernel_panic("trap gate initialization failed");
    }
    KPRINT("  [TRAP] Ready.\n");

    if (boot_policy->enable_ai_governor != 0U) {
      kernel_ai_governor_init();
    }

    KPRINT("  [CPU] Enabling interrupts...\n");
    hal_cpu_enable_interrupts();
    KPRINT("  [CPU] Interrupts enabled.\n");

    boot_selftest_report_t report;
    boot_selftest_run_stage(BOOT_TEST_STAGE_PLATFORM, &report);
}

extern void kernel_run_boot_tests(void);
extern void hello_world_app(void);
extern void kernel_tester_app(void);
extern void bharat_demo_app_legacy(void);
extern void bharat_demo_app(void);

extern int boot_video_map(const boot_info_t *boot);

static bool runtime_try_boot_video(const boot_info_t *boot) {
    if (!boot || boot->console.type != BOOT_CONSOLE_FRAMEBUFFER) {
        return false;
    }

    if (boot_video_map(boot) != 0) {
        KPRINT("  [UI] Boot video explicitly mapping failed. Falling back to text.\n");
        return false;
    }

    KPRINT("  [UI] Boot video mapped safely.\n");
    return true;
}

static void runtime_maybe_boot_gui(bool video_mapped) {
#if BHARAT_BOOT_GUI
    if (video_mapped) {
        KPRINT("  [UI] Initializing boot GUI...\n");
        if (boot_gui_run() == 0) {
            KPRINT("  [UI] Boot framebuffer active.\n");
        } else {
            KPRINT("  [UI] Boot GUI not available, text mode only.\n");
        }
    } else {
        KPRINT("  [UI] Video handoff inactive or invalid. Booting serial UI fallback only.\n");
    }
#else
    (void)video_mapped;
#endif /* BHARAT_BOOT_GUI */
}

// Forward declaration for user-space initialization stub
extern void kernel_start_init_service(void);

static void runtime_enter_normal(const boot_info_t *boot) {
    bool video_mapped = runtime_try_boot_video(boot);
    runtime_maybe_boot_gui(video_mapped);

    boot_selftest_report_t report;
    boot_selftest_run_stage(BOOT_TEST_STAGE_RUNTIME, &report);

    KPRINT("  [BOOT] Spawning first system service (sysmgr)...\n");
    kernel_start_init_service();

    // Controlled idle
    while (1) {
        hal_cpu_halt();
    }
}

static void runtime_enter_diagnostic(const boot_info_t *boot) {
    bool video_mapped = runtime_try_boot_video(boot);
    runtime_maybe_boot_gui(video_mapped);

    boot_selftest_report_t report;
    boot_selftest_run_stage(BOOT_TEST_STAGE_RUNTIME, &report);

    kernel_tester_app();

    // Controlled idle
    while (1) {
        hal_cpu_halt();
    }
}

static void runtime_enter_recovery(const boot_info_t *boot) {
    (void)boot;
    // Minimal recovery path. No generic tests, no GUI by default.
    KPRINT("  [BOOT] Recovery mode active.\n");

    boot_selftest_report_t report;
    boot_selftest_run_stage(BOOT_TEST_STAGE_RUNTIME, &report);

    while (1) {
        hal_cpu_halt();
    }
}

static void runtime_enter_manufacturing(const boot_info_t *boot) {
    bool video_mapped = runtime_try_boot_video(boot);
    runtime_maybe_boot_gui(video_mapped);

    boot_selftest_report_t report;
    boot_selftest_run_stage(BOOT_TEST_STAGE_RUNTIME, &report);

    while (1) {
        hal_cpu_halt();
    }
}

static void runtime_enter_benchmark(const boot_info_t *boot) {
    (void)boot;
    // Optional video map here but user said defaulting to text is cleaner
    // No generic boot test sweep by default

    boot_selftest_report_t report;
    boot_selftest_run_stage(BOOT_TEST_STAGE_RUNTIME, &report);

    while (1) {
        hal_cpu_halt();
    }
}

static void runtime_enter_legacy_bringup(const boot_info_t *boot) {
    bool video_mapped = runtime_try_boot_video(boot);
    runtime_maybe_boot_gui(video_mapped);

    boot_selftest_report_t report;
    boot_selftest_run_stage(BOOT_TEST_STAGE_RUNTIME, &report);

    kernel_run_boot_tests();

    /* Apps */
    hello_world_app();
    kernel_tester_app();
    bharat_demo_app_legacy();
    bharat_demo_app();

    while (1) {
      // Background AI
      hal_cpu_halt();
    }
}

void boot_common_runtime(const boot_info_t *boot) {
    bharat_boot_mode_t mode = (bharat_boot_mode_t)boot->selected_mode; // Resolved earlier

    KPRINT("  [BOOT] Runtime mode: ");
    KPRINT(bharat_boot_mode_name(mode));
    KPRINT("\n");

    switch (mode) {
        case BHARAT_BOOT_MODE_NORMAL:
        default:
            KPRINT("  [BOOT] Entering normal runtime\n");
            runtime_enter_normal(boot);
            break;
        case BHARAT_BOOT_MODE_DIAGNOSTIC:
            KPRINT("  [BOOT] Entering diagnostic runtime\n");
            runtime_enter_diagnostic(boot);
            break;
        case BHARAT_BOOT_MODE_RECOVERY:
            KPRINT("  [BOOT] Entering recovery runtime\n");
            runtime_enter_recovery(boot);
            break;
        case BHARAT_BOOT_MODE_MANUFACTURING:
            KPRINT("  [BOOT] Entering manufacturing runtime\n");
            runtime_enter_manufacturing(boot);
            break;
        case BHARAT_BOOT_MODE_BENCHMARK:
            KPRINT("  [BOOT] Entering benchmark runtime\n");
            runtime_enter_benchmark(boot);
            break;
        case BHARAT_BOOT_MODE_LEGACY_BRINGUP:
            KPRINT("  [BOOT] Entering legacy bring-up runtime\n");
            runtime_enter_legacy_bringup(boot);
            break;
    }

    // Safety halt if runtime entry returns
    while(1) {
        hal_cpu_halt();
    }
}
