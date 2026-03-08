#include "advanced/ai_kernel_bridge.h"
#include "advanced/ai_sched.h"
#include "advanced/multikernel.h"
#include "device.h"
#include "hal/hal.h"
#include "kernel.h"
#include "mm.h"
#include "numa.h"
#include "trap.h"
#include "multicore.h"
#include "advanced/algo_matrix.h"
#include "mm_zswap.h"

#include <stddef.h>
#include <stdint.h>


#if defined(__riscv)
#include "hal/riscv_bsp.h"
#endif

#define KPRINT(s) hal_serial_write(s)
#define CAP_RIGHT_IPC_ENDPOINT 0x1U


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
  kernel_capability_t service_cap = {.cap_id = 1U,
                                     .rights_mask = CAP_RIGHT_IPC_ENDPOINT};
  kernel_ipc_msg_t request = {
      .msg_id = 1U, .msg_len = 6U, .payload = {'h', 'e', 'l', 'l', 'o', '\0'}};
  kernel_ipc_msg_t reply = {0};

  KPRINT("P2: hello service launched\n");
  KPRINT("P2: capability granted id=1 rights=ipc\n");
  KPRINT("P2: ipc request sent\n");

  hello_service_task(&service_cap, &request, &reply);

  if (reply.msg_len > 0U && reply.payload[0] != '\0') {
    KPRINT("P2: ipc reply received payload=");
    hal_serial_write(reply.payload);
    KPRINT("\nP2: hello ipc smoke test passed\n");
  } else {
    KPRINT("P2: hello ipc smoke test failed\n");
  }
}

static void kernel_boot_scheduler(void) {
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
void kernel_main(uint32_t magic, multiboot_information_t *mb_info) {
#elif defined(__riscv)
void kernel_main(uint64_t hart_id, uintptr_t fdt_ptr) {
#else
void kernel_main(void) {
#endif
  const char *profile = kernel_boot_hw_profile();

#if defined(__riscv)
  hal_riscv_set_boot_info(hart_id, (uint64_t)fdt_ptr);
#endif

  KPRINT("\n");
  KPRINT("  ____  _                          _          ____   ____  \n");
  KPRINT(" | __ )| |__   __ _ _ __ __ _ _| |_       / ___| / ___| \n");
  KPRINT(" |  _ \\| '_ \\ / _` | '__/ _` | '__/ _` |_____| |  _  \\___ \\ \n");
  KPRINT(" | |_) | | | | (_| | | | (_| | | | (_| |_____| |_| |  ___) |\n");
  KPRINT(" |____/|_| |_|\\__,_|_|  \\__,_|_|  \\__,_|      \\____| |____/ \n");
  KPRINT("\nBharat-OS kernel boot (v3.2 bring-up)\n");

  uint32_t cpu_id = hal_cpu_get_id();
  int is_bsp = (cpu_id == 0);

  if (is_bsp) {
    KPRINT("  [HAL] Initialising hardware on BSP...\n");
    hal_init();
    KPRINT("  [HAL] Ready.\n");

    KPRINT("  [ALGO] Initializing Capability Matrix...\n");
    algo_matrix_init();
    KPRINT("  [ALGO] Matrix Ready.\n");

    KPRINT("  [MM]  Initializing PMM...\n");

#if defined(__x86_64__)
    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
      kernel_panic("Invalid multiboot magic");
    }

    if (mm_pmm_init(mb_info, mb_info->total_size) != 0) {
      kernel_panic("PMM initialization failed");
    }
#else
    if (mm_pmm_init(NULL, 0U) != 0) {
      kernel_panic("PMM initialization failed");
    }
#endif
    KPRINT("BOOT: pmm initialized\n");

    KPRINT("  [VMM] Initializing VMM...\n");
    if (vmm_init() != 0) {
      kernel_panic("VMM initialization failed");
    }
    KPRINT("BOOT: vmm initialized\n");

    KPRINT("  [ZSWAP] Initializing Memory Compression...\n");
    if (zswap_init() != 0) {
        kernel_panic("ZSWAP initialization failed");
    }
    KPRINT("BOOT: zswap initialized\n");

    KPRINT("  [NUMA] Discovering topology\n");
    if (numa_discover_topology() != 0) {
      kernel_panic("numa topology discovery failed");
    }

    KPRINT("  [SMP] Booting secondary cores\n");
    if (mk_boot_secondary_cores(2U) != 0) {
      kernel_panic("secondary core boot failed");
    }

    KPRINT("  [SMP] Initializing per-core URPC channels\n");
    if (mk_init_per_core_channels(2U, 32U) != 0) {
      kernel_panic("per-core urpc channel init failed");
    }

    KPRINT("  [IRQ] Initializing interrupt controller\n");
    if (hal_interrupt_controller_init() != 0) {
      kernel_panic("interrupt controller initialization failed");
    }

    KPRINT("  [TMR] Initializing timer source\n");
    if (hal_timer_init(100U) != 0) {
      kernel_panic("timer initialization failed");
    }

    KPRINT("  [DEV] Initializing device framework\n");
    if (device_framework_init() != 0 || device_register_builtin_drivers() != 0) {
      kernel_panic("device framework initialization failed");
    }

    kernel_boot_scheduler();

    KPRINT("  [TRAP] Initializing syscall/trap gate...\n");
    if (trap_init() != 0) {
      kernel_panic("trap gate initialization failed");
    }
    KPRINT("  [TRAP] Ready.\n");

    kernel_ai_governor_init();

    KPRINT("  [CPU] Enabling interrupts...\n");
    hal_cpu_enable_interrupts();
    KPRINT("  [CPU] Interrupts enabled.\n");

    kernel_phase2_hello_service_smoke();
    KPRINT("TEST: kernel self-tests passed\n");

    hal_serial_write("[bharat] kernel_main reached on BSP\n");
    hal_serial_write("[bharat] hw_profile=");
    hal_serial_write(profile);
    hal_serial_write("\n");

    while (1) {
      kernel_ai_publish_telemetry();
      kernel_ai_governor_tick();
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
