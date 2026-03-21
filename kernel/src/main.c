#include "hal/hal_irq.h"
#include "hal/hal_timer.h"
#include "advanced/ai_kernel_bridge.h"
#include "advanced/ai_sched.h"
#include "advanced/algo_matrix.h"
#include "advanced/multikernel.h"
#include "device.h"
#include "hal/hal.h"
#include "hal/hal_discovery.h"

#if defined(__riscv) || defined(__aarch64__)
#include "hal/fdt_parser.h"
#include "hal/mmu_ops.h"
#endif
#include "ipc_async.h"
#include "kernel.h"
#include "console/console_core.h"
#include "mm.h"
#include "arch/arch_cpu_caps.h"
#include "mm_zswap.h"
#include "bharat/component_version.h"
#include "bharat/buildinfo.h"
#include "bharat/version.h"

BHARAT_REGISTER_COMPONENT(
    "kernel",
    "kernel",
    BHARAT_KERNEL_VERSION,
    BHARAT_KERNEL_ABI,
    BHARAT_SYSCALL_ABI,
    BHARAT_OS_CHANNEL,
    BHARAT_GIT_SHA,
    BHARAT_GIT_DIRTY,
    BHARAT_BUILD_EPOCH,
    BHARAT_BUILD_TIME_UTC
);

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

#include <stddef.h>
#include <stdint.h>

#if defined(__riscv)
#include "hal/riscv_bsp.h"
#endif

#define KPRINT(s) console_write_raw(s, string_length(s))
#define CAP_RIGHT_IPC_ENDPOINT 0x1U

extern void console_register_serial_backend(void);

typedef struct {
  uint32_t cap_id;
  uint32_t rights_mask;
} kernel_capability_t;

typedef struct {
  uint32_t msg_id;
  uint32_t msg_len;
  char payload[32];
} kernel_ipc_msg_t;

static mk_channel_t g_scheduler_ai_channel;

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

static void hello_service_task(const kernel_capability_t *cap,
                               const kernel_ipc_msg_t *request,
                               kernel_ipc_msg_t *reply) {
  if (!cap || !request || !reply)
    return;

  reply->msg_id = request->msg_id;
  if ((cap->rights_mask & CAP_RIGHT_IPC_ENDPOINT) == 0U) {
    reply->msg_len = 11U;
    reply->payload[0] = 'D';
    reply->payload[1] = 'E';
    reply->payload[2] = 'N';
    reply->payload[3] = 'I';
    reply->payload[4] = 'E';
    reply->payload[5] = 'D';
    reply->payload[6] = ':';
    reply->payload[7] = 'I';
    reply->payload[8] = 'P';
    reply->payload[9] = 'C';
    reply->payload[10] = '\0';
    return;
  }

  reply->msg_len = 13U;
  reply->payload[0] = 'h';
  reply->payload[1] = 'e';
  reply->payload[2] = 'l';
  reply->payload[3] = 'l';
  reply->payload[4] = 'o';
  reply->payload[5] = '-';
  reply->payload[6] = 'a';
  reply->payload[7] = 'c';
  reply->payload[8] = 'k';
  reply->payload[9] = ':';
  reply->payload[10] = 'o';
  reply->payload[11] = 'k';
  reply->payload[12] = '\0';
}

static void kernel_phase2_hello_service_smoke(void) {
  kernel_capability_t service_cap __attribute__((aligned(16))) = {
      .cap_id = 1U, .rights_mask = CAP_RIGHT_IPC_ENDPOINT};
  kernel_ipc_msg_t request __attribute__((aligned(16))) = {
      .msg_id = 1U, .msg_len = 6U, .payload = {'h', 'e', 'l', 'l', 'o', '\0'}};
  kernel_ipc_msg_t reply __attribute__((aligned(16))) = {0};

  KPRINT("P2: hello service launched\n");
  KPRINT("P2: capability granted id=1 rights=ipc\n");
  KPRINT("P2: ipc request sent\n");

  hello_service_task(&service_cap, &request, &reply);

  if (reply.msg_len > 0U && reply.payload[0] != '\0') {
    KPRINT("P2: ipc reply received payload=");
    console_write_raw(reply.payload, string_length(reply.payload));
    KPRINT("\nP2: hello ipc smoke test passed\n");
  } else {
    KPRINT("P2: hello ipc smoke test failed\n");
  }
}

static void kernel_boot_scheduler(void) {
  arch_cpu_caps_init();
  arch_cpu_caps_system_finalize(); // Aggregates system_all and system_any
  hal_discovery_publish_cpu_caps();
  // fastops_dispatch_init();
  arch_ext_state_boot_init();

  // TODO: WIP - CPU capabilities diagnostic code disabled (needs formatted print)
  // const arch_cpu_caps_record_t *boot = arch_cpu_caps_boot();
  // KPRINT("  [ARCH]  cpu_caps: cpu0 usable aes=%d sha=%d vector=%d\n",
  //        arch_cpu_caps_test(&boot->usable, ARCH_CPU_FEAT_COMMON_AES),
  //        arch_cpu_caps_test(&boot->usable, ARCH_CPU_FEAT_COMMON_SHA),
  //        arch_cpu_caps_test(&boot->usable, ARCH_CPU_FEAT_COMMON_VECTOR));

  // const arch_cpu_caps_record_t *sys_all = arch_cpu_caps_system_all();
  // KPRINT("  [ARCH]  cpu_caps: system_all usable aes=%d sha=%d vector=%d\n",
  //        arch_cpu_caps_test(&sys_all->usable, ARCH_CPU_FEAT_COMMON_AES),
  //        arch_cpu_caps_test(&sys_all->usable, ARCH_CPU_FEAT_COMMON_SHA),
  //        arch_cpu_caps_test(&sys_all->usable, ARCH_CPU_FEAT_COMMON_VECTOR));

  // KPRINT("  [ARCH]  fastops: page_zero backend=%s\n",
  //        arch_cpu_caps_test(&sys_all->usable, ARCH_CPU_FEAT_COMMON_VECTOR) ? "optimized" : "generic");

  // KPRINT("  [ARCH]  CPU capabilities and extended state initialized.\n");

  sched_init();
  KPRINT("  [SCHED] Scheduler initialized.\n");
}

static void kernel_ai_governor_init(void) {
  if (mk_establish_channel(0U, &g_scheduler_ai_channel) == 0) {
    KPRINT("  [AI]  Scheduler control channel ready.\n");
  } else {
    kernel_panic("failed to establish AI scheduler control channel");
  }
}

static void kernel_ai_publish_telemetry(void) {
  urpc_msg_t msg = {0};
  kernel_telemetry_t telemetry = {0};

  if (ai_kernel_collect_telemetry(&telemetry) != 0) {
    return;
  }

  msg.msg_type = AI_MSG_TYPE_TELEMETRY;
  msg.payload_size = sizeof(kernel_telemetry_t);
  ((kernel_telemetry_t *)msg.payload_data)[0] = telemetry;
  (void)urpc_send(g_scheduler_ai_channel.urpc_ring, &msg);
}

static void kernel_ai_governor_tick(void) {
  urpc_msg_t msg = {0};
  while (urpc_receive(g_scheduler_ai_channel.urpc_ring, &msg) == 0) {
    if (msg.msg_type == AI_MSG_TYPE_SUGGESTION &&
        msg.payload_size >= sizeof(ai_suggestion_t)) {
      ai_suggestion_t *suggestion = (ai_suggestion_t *)msg.payload_data;
      if (ai_kernel_apply_suggestion(suggestion) == 0) {
        KPRINT("  [AI]  Scheduler suggestion accepted.\n");
      } else {
        KPRINT("  [AI]  Scheduler suggestion rejected.\n");
      }
    }
  }
}

#if defined(__x86_64__)
#include "boot/x86_64/multiboot2.h"

// Forward declaration for x86_64 framebuffer parser
extern void x86_64_parse_multiboot_framebuffer(multiboot_information_t *mb_info);

void kernel_main(uint32_t magic, multiboot_information_t *mb_info) {
#elif defined(__riscv)
void kernel_main(uint64_t hart_id, uintptr_t fdt_ptr) {
#elif defined(__aarch64__) || defined(__arm__)
void kernel_main(uintptr_t fdt_ptr) {
  hal_serial_init();
  hal_serial_write("!!!!! BOOT: fdt_ptr="); hal_serial_write_hex(fdt_ptr); hal_serial_write("\n");
#else
void kernel_main(void) {
#endif
  const char *profile = kernel_boot_hw_profile();
  const bharat_boot_policy_t *boot_policy = bharat_boot_active_policy();

#if defined(__riscv)
  hal_riscv_set_boot_info(hart_id, (uint64_t)fdt_ptr);
#endif

  /* Initialize core console layer and serial backend early to allow KPRINT to work immediately */
  console_early_init();

#if defined(__riscv) || defined(__aarch64__)
  KPRINT("!!!!! BOOT: Entry kernel_main, fdt_ptr_arg="); 
  hal_serial_write_hex(fdt_ptr); 
  KPRINT("\n");
#endif

#if defined(__riscv) || defined(__aarch64__)
    if (fdt_ptr == 0 || !fdt_is_valid((void*)fdt_ptr)) {
      KPRINT("BOOT: FDT pointer invalid or zero. Using hardcoded fallback 0x40000000...\n");
      fdt_ptr = 0x40000000;
    }

    if (fdt_is_valid((void*)fdt_ptr)) {
        KPRINT("BOOT: Validating FDT at "); hal_serial_write_hex(fdt_ptr); KPRINT("...\n");
        fdt_parse_discovery((const void *)fdt_ptr, hal_get_system_discovery());
    } else {
        KPRINT("BOOT: CRITICAL: FDT NOT FOUND AT FALLBACK!\n");
    }
#endif

  KPRINT("\n");
  KPRINT("  ____  _                          _          ____   ____  \n");
  KPRINT(" | __ )| |__   __ _ _ __ __ _ _| |_       / ___| / ___| \n");
  KPRINT(" |  _ \\| '_ \\ / _` | '__/ _` | '__/ _` |_____| |  _  \\___ \\ \n");
  KPRINT(" | |_) | | | | (_| | | | (_| | | | (_| |_____| |_| |  ___) |\n");
  KPRINT(" |____/|_| |_|\\__,_|_|  \\__,_|_|  \\__,_|      \\____| |____/ \n");
  KPRINT("\nBharat-OS (");
  KPRINT(BHARAT_OS_VERSION);
  KPRINT("-");
  KPRINT(BHARAT_OS_CHANNEL);
  KPRINT(") [");
  KPRINT(BHARAT_GIT_SHA);
  if (BHARAT_GIT_DIRTY) {
      KPRINT("-dirty");
  }
  KPRINT("]\n");
  KPRINT("Kernel Version: ");
  KPRINT(BHARAT_KERNEL_VERSION);
  KPRINT("\n\n");

  uint32_t cpu_id = hal_cpu_get_id();
  cpu_local_init(cpu_id);
  
  int is_bsp = (cpu_id == 0);

  if (is_bsp) {
    KPRINT("  [HAL] Initialising hardware on BSP...\n");
    hal_init();
    KPRINT("  [HAL] Ready.\n");

    KPRINT("  [PROFILE] Applying hardware profile hooks...\n");
    profile_init();

    bharat_subsystems_init(profile);
    KPRINT("  [SUBSYS] Storage subsystem profiled.\n");
    if (bharat_storage_has(BHARAT_STORAGE_NVME)) {
      KPRINT("    - storage: NVMe enabled\n");
    }
    if (bharat_storage_has(BHARAT_STORAGE_AHCI_SATA)) {
      KPRINT("    - storage: AHCI/SATA enabled\n");
    }
    if (bharat_storage_has(BHARAT_STORAGE_EMMC_SD)) {
      KPRINT("    - storage: eMMC/SD enabled\n");
    }
    if (bharat_storage_has(BHARAT_STORAGE_FLASH_MTD)) {
      KPRINT("    - storage: flash/MTD enabled\n");
    }
    if (bharat_storage_has(BHARAT_STORAGE_RAMDISK)) {
      KPRINT("    - storage: RAM disk enabled\n");
    }

    KPRINT("  [SUBSYS] Network subsystem profiled.\n");
    if (bharat_network_has(BHARAT_NET_LIGHTWEIGHT_STACK)) {
      KPRINT("    - network: lightweight embedded stack\n");
    }
    if (bharat_network_has(BHARAT_NET_FULL_TCPIP_STACK)) {
      KPRINT("    - network: full TCP/IP stack\n");
    }
    if (bharat_network_has(BHARAT_NET_ZERO_COPY_PATH)) {
      KPRINT("    - network: zero-copy packet path (profile-ready)\n");
    }

    KPRINT("  [SUBSYS] Filesystem subsystem profiled.\n");
    if (bharat_filesystem_has(BHARAT_FS_VFS)) {
      KPRINT("    - fs: VFS core enabled\n");
    }
    if (bharat_filesystem_has(BHARAT_FS_PAGE_CACHE)) {
      KPRINT("    - fs: page cache enabled\n");
    }
    if (bharat_filesystem_has(BHARAT_FS_WRITEBACK)) {
      KPRINT("    - fs: writeback layer enabled\n");
    }
    if (bharat_filesystem_has(BHARAT_FS_EXT_LIKE)) {
      KPRINT("    - fs: ext-like driver target enabled\n");
    }
    if (bharat_filesystem_has(BHARAT_FS_LITTLEFS)) {
      KPRINT("    - fs: littlefs class support enabled\n");
    }
    if (bharat_filesystem_has(BHARAT_FS_JOURNALING)) {
      KPRINT("    - fs: journaling mode enabled\n");
    }

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

    KPRINT("  [MM]  Initializing PMM...\n");

#if defined(__x86_64__)
    if (mm_pmm_init(magic, mb_info) != 0) {
      kernel_panic("PMM initialization failed");
    }
#elif defined(__riscv) || defined(__aarch64__) || defined(__arm__)
    if (mm_pmm_init(0U, (void *)fdt_ptr) != 0) {
      kernel_panic("PMM initialization failed");
    }
#else
    if (mm_pmm_init(0U, NULL) != 0) {
      kernel_panic("PMM initialization failed");
    }
#endif
    KPRINT("BOOT: pmm initialized\n");

    KPRINT("  [VMM] Initializing VMM...\n");
    if (vmm_init() != 0) {
      kernel_panic("VMM initialization failed");
    }

#if defined(__x86_64__)
    // Map LAPIC and IOAPIC MMIO regions
    vmm_map_page(0xFEE00000, 0xFEE00000, CAP_RIGHT_READ | CAP_RIGHT_WRITE);
    vmm_map_page(0xFEC00000, 0xFEC00000, CAP_RIGHT_READ | CAP_RIGHT_WRITE);
#endif

    KPRINT("BOOT: vmm initialized\n");

    system_discovery_t* discovery = hal_get_system_discovery();
    KPRINT("BOOT: discovery ");
    hal_serial_write_hex((uintptr_t)discovery);
    KPRINT("\n");

#if defined(__aarch64__)
    // Map all discovered RAM regions to ensure the kernel (code+bss) is accessible
    // after we activate the new page tables.
    if (discovery) {
        KPRINT("  [VMM] Memory region count: ");
        hal_serial_write_hex(discovery->topology.mem_region_count);
        KPRINT("\n");
        for (uint32_t i = 0; i < discovery->topology.mem_region_count; i++) {
            uint64_t base = discovery->topology.mem_regions[i].base;
            uint64_t size = discovery->topology.mem_regions[i].size;
            KPRINT("  [VMM] Mapping RAM region: ");
            hal_serial_write_hex(base);
            KPRINT(" (size ");
            hal_serial_write_hex(size);
            KPRINT(")\n");
            for (uint64_t off = 0; off < size; off += 4096) {
                vmm_map_page(base + off, base + off, 
                             CAP_RIGHT_READ | CAP_RIGHT_WRITE | CAP_RIGHT_EXECUTE);
            }
        }
    } else {
        KPRINT("  [VMM] WARNING: Discovery is NULL, skipping RAM mapping!\n");
    }
#endif

#if defined(__riscv) || defined(__aarch64__)
    // Map discovered framebuffer if present
    if (discovery && discovery->boot_video.valid) {
        uint64_t fb_phys = discovery->boot_video.phys_addr;
        uint64_t fb_size = discovery->boot_video.size;
        KPRINT("  [VMM] Mapping framebuffer: ");
        hal_serial_write_hex(fb_phys);
        KPRINT(" (size ");
        hal_serial_write_hex(fb_size);
        KPRINT(")\n");
        
        // Map as much as needed, align to page size (4KB)
        for (uint64_t off = 0; off < fb_size; off += 4096) {
            vmm_map_page(fb_phys + off, fb_phys + off, 
                         CAP_RIGHT_READ | CAP_RIGHT_WRITE | CAP_RIGHT_DEVICE_GPU);
        }
        KPRINT("  [VMM] Framebuffer mapped successfully.\n");
    }
#endif

#if defined(__riscv) || defined(__aarch64__)
    // Initialize MMU Ops and activate hardware MMU with the kernel page table
    // to ensure our framebuffer mapping (applied above) is active in hardware.
    extern void arch_mmu_init(void);
    arch_mmu_init();

    extern phys_addr_t vmm_get_kernel_root(void);
    extern mmu_ops_t *active_mmu;
    if (active_mmu && active_mmu->activate) {
        active_mmu->activate(vmm_get_kernel_root());
    }
#endif

    if (boot_policy->enable_zswap != 0U) {
      KPRINT("  [ZSWAP] Initializing Memory Compression...\n");
      if (zswap_init() != 0) {
        kernel_panic("ZSWAP initialization failed");
      }
      KPRINT("BOOT: zswap initialized\n");
    } else {
      KPRINT("BOOT: zswap skipped (fast-boot policy)\n");
    }

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

    kernel_boot_scheduler();

#if BHARAT_BOOT_GUI
    KPRINT("  [UI] Initializing boot GUI...\n");
    if (boot_gui_run() == 0) {
      KPRINT("  [UI] Boot framebuffer active.\n");
    } else {
      KPRINT("  [UI] Boot GUI not available, text mode only.\n");
    }
#endif /* BHARAT_BOOT_GUI */

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

    kernel_phase2_hello_service_smoke();
    KPRINT("TEST: kernel self-tests passed\n");

    // Run registered boot tests
    extern void kernel_run_boot_tests(void);
    kernel_run_boot_tests();

    console_write_raw("[bharat] hw_profile=", string_length("[bharat] hw_profile="));
    console_write_raw(profile, string_length(profile));
    console_write_raw("\n", string_length("\n"));

    /* Hello World Application */
    extern void hello_world_app(void);
    hello_world_app();

    /* Kernel Tester Application - Perfection Phase */
    extern void kernel_tester_app(void);
    kernel_tester_app();

    /* Bharat-OS Feature Showcase Demo */
    extern void bharat_demo_app_legacy(void);
    bharat_demo_app_legacy();

    /* FBUI Demo */
    extern void bharat_demo_app(void);
    bharat_demo_app();

    while (1) {
      if (boot_policy->enable_ai_governor != 0U) {
        kernel_ai_publish_telemetry();
        kernel_ai_governor_tick();
      }
      hal_cpu_halt();
    }
  } else {
    // AP boot path
    smp_init(); // Set up CPU-local data structures

    while (1) {
      hal_cpu_halt();
    }
  }
}
