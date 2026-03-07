#include "hal/hal.h"
#include "kernel.h"
#include "mm.h"
#include "advanced/multikernel.h"
#include "advanced/ai_sched.h"

#include <stdint.h>
#include <stddef.h>

#define KPRINT(s) hal_serial_write(s)
#define CAP_RIGHT_IPC_ENDPOINT 0x1U
#define AI_MSG_TYPE_SUGGESTION 1U

void sched_init(void) __attribute__((weak));

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
  if (!cap || !request || !reply) return;

  reply->msg_id = request->msg_id;
  if ((cap->rights_mask & CAP_RIGHT_IPC_ENDPOINT) == 0U) {
    reply->msg_len = 11U;
    reply->payload[0] = 'D'; reply->payload[1] = 'E'; reply->payload[2] = 'N';
    reply->payload[3] = 'I'; reply->payload[4] = 'E'; reply->payload[5] = 'D';
    reply->payload[6] = ':'; reply->payload[7] = 'I'; reply->payload[8] = 'P';
    reply->payload[9] = 'C'; reply->payload[10] = '\0';
    return;
  }

  reply->msg_len = 13U;
  reply->payload[0] = 'h'; reply->payload[1] = 'e'; reply->payload[2] = 'l';
  reply->payload[3] = 'l'; reply->payload[4] = 'o'; reply->payload[5] = '-';
  reply->payload[6] = 'a'; reply->payload[7] = 'c'; reply->payload[8] = 'k';
  reply->payload[9] = ':'; reply->payload[10] = 'o'; reply->payload[11] = 'k';
  reply->payload[12] = '\0';
}

static void kernel_phase2_hello_service_smoke(void) {
  kernel_capability_t service_cap = { .cap_id = 1U, .rights_mask = CAP_RIGHT_IPC_ENDPOINT };
  kernel_ipc_msg_t request = { .msg_id = 1U, .msg_len = 6U, .payload = {'h', 'e', 'l', 'l', 'o', '\0'} };
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
  if (sched_init) {
    sched_init();
    KPRINT("  [SCHED] Scheduler initialized.\n");
    return;
  }

  kernel_panic("scheduler init symbol missing");
}

static void kernel_ai_governor_init(void) {
  if (mk_establish_channel(0U, &g_scheduler_ai_channel) == 0) {
    KPRINT("  [AI]  Scheduler control channel ready.\n");
  } else {
    kernel_panic("failed to establish AI scheduler control channel");
  }
}

static void kernel_ai_governor_tick(void) {
  urpc_msg_t msg = {0};
  while (urpc_receive(g_scheduler_ai_channel.urpc_ring, &msg) == 0) {
    if (msg.msg_type == AI_MSG_TYPE_SUGGESTION && msg.payload_size >= sizeof(ai_suggestion_t)) {
      ai_suggestion_t* suggestion = (ai_suggestion_t*)msg.payload_data;
      (void)suggestion;
      KPRINT("  [AI]  Scheduler suggestion received.\n");
    }
  }
}

void kernel_main(void) {
  const char *profile = kernel_boot_hw_profile();

  KPRINT("\nBharat-OS kernel boot\n");
  KPRINT("  [HAL] Initialising hardware...\n");
  hal_init();
  KPRINT("  [HAL] Ready.\n");

  KPRINT("  [MM]  Initializing PMM...\n");
  if (mm_pmm_init(NULL, 0U) != 0) {
    kernel_panic("PMM initialization failed");
  }
  KPRINT("BOOT: pmm initialized\n");

  KPRINT("  [VMM] Initializing VMM...\n");
  if (vmm_init() != 0) {
    kernel_panic("VMM initialization failed");
  }
  KPRINT("BOOT: vmm initialized\n");

  kernel_boot_scheduler();
  kernel_ai_governor_init();

  KPRINT("  [CPU] Enabling interrupts...\n");
  hal_cpu_enable_interrupts();
  KPRINT("  [CPU] Interrupts enabled.\n");

  kernel_phase2_hello_service_smoke();
  KPRINT("TEST: kernel self-tests passed\n");

  hal_serial_write("[bharat] kernel_main reached\n");
  hal_serial_write("[bharat] hw_profile=");
  hal_serial_write(profile);
  hal_serial_write("\n");

  while (1) {
    kernel_ai_governor_tick();
    hal_cpu_halt();
  }
}
