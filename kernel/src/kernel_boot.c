#include "hal/hal_irq.h"
#include "hal/hal_timer.h"
#include "advanced/ai_kernel_bridge.h"
#include "advanced/ai_sched.h"
#include "advanced/algo_matrix.h"
#include "advanced/multikernel.h"
#include "device.h"
#include "hal/hal.h"
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
#include "profile.h"
#include "secure_boot.h"
#include "security/audit.h"
#include "security/credentials.h"
#include "security/isolation.h"
#include "security/policy.h"
#include "subsystem_profile.h"
#include "trap.h"
#include "boot/boot_args.h"
#include "bharat/boot_info.h"
#include "display/boot_gui_init.h"
#include "tests/ktest.h"
#include <bharat/cpu_local.h>
#include "arch/arch_ext_state.h"
#include "arch/arch_cpu_caps.h"
#include "arch/arch_caps.h"
#include "arch/memops.h"

#define KPRINT(s) console_write_raw(s, string_length(s))

static const char *kernel_boot_hw_profile(void) {
#if defined(BHARAT_BOOT_HW_PROFILE_desktop)
  return "desktop";
#elif defined(BHARAT_BOOT_HW_PROFILE_server)
  return "server";
#elif defined(BHARAT_BOOT_HW_PROFILE_vm)
  return "vm";
#elif defined(BHARAT_BOOT_HW_PROFILE_laptop)
  return "laptop";
#elif defined(BHARAT_BOOT_HW_PROFILE_mobile)
  return "mobile";
#elif defined(BHARAT_BOOT_HW_PROFILE_datacenter)
  return "datacenter";
#elif defined(BHARAT_BOOT_HW_PROFILE_network_appliance)
  return "network_appliance";
#else
  return "generic";
#endif
}

void boot_common_early(const boot_info_t *boot) {
    (void)boot;
    const char *profile = kernel_boot_hw_profile();

    hal_init();

    KPRINT("\n");
    KPRINT("  ____  _                          _          ____   ____  \n");
    KPRINT(" | __ )| |__   __ _ _ __ __ _ _| |_       / ___| / ___| \n");
    KPRINT(" |  _ \\| '_ \\ / _` | '__/ _` | '__/ _` |_____| |  _  \\___ \\ \n");
    KPRINT(" | |_) | | | | (_| | | | (_| | | | (_| |_____| |_| |  ___) |\n");
    KPRINT(" |____/|_| |_|\\__,_|_|  \\__,_|_|  \\__,_|      \\____| |____/ \n");
    KPRINT("\nBharat-OS\n");

    KPRINT("  [HAL] Initialising hardware on BSP...\n");
    hal_discovery_init(boot);
    KPRINT("  [HAL] Ready.\n");

    KPRINT("  [PROFILE] Applying hardware profile hooks...\n");
    profile_init();

    // Print selected capabilities
    arch_caps_t arch_caps = arch_get_caps();

    // Initialise architecture memory operations (cached capability check)
    arch_memops_init();

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

    bharat_subsystems_init(profile);
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

    extern void arch_mmu_init(void);
    arch_mmu_init();

    extern void hal_mmu_final_setup(void);
    hal_mmu_final_setup();
    KPRINT("  [VMM] Architecture MMU mappings configured.\n");

    const bharat_boot_policy_t *boot_policy = bharat_boot_active_policy();
    if (boot_policy->enable_zswap != 0U) {
      KPRINT("  [ZSWAP] Initializing Memory Compression...\n");
      if (zswap_init() != 0) {
        kernel_panic("ZSWAP initialization failed");
      }
      KPRINT("BOOT: zswap initialized\n");
    }
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
}

extern void kernel_run_boot_tests(void);
extern void hello_world_app(void);
extern void kernel_tester_app(void);
extern void bharat_demo_app_legacy(void);
extern void bharat_demo_app(void);

extern int boot_video_map(const boot_info_t *boot);

void boot_common_runtime(const boot_info_t *boot) {
    // Track video mapped status tightly.
    bool video_mapped = false;

    // Map video explicitly after VMM if valid to prevent page faults
    if (boot->video.valid) {
        if (boot_video_map(boot) != 0) {
            KPRINT("  [UI] Boot video explicitly mapping failed. Falling back to text.\n");
        } else {
            video_mapped = true;
            KPRINT("  [UI] Boot video mapped safely.\n");
        }
    }

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
#endif /* BHARAT_BOOT_GUI */

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
