#include "hal/hal.h"
#include "kernel.h"

#include <stdint.h>

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
  if (!cap || !request || !reply) {
    return;
  }

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
  kernel_capability_t service_cap;
  kernel_ipc_msg_t request;
  kernel_ipc_msg_t reply;

  KPRINT("P2: hello service launched\n");

  service_cap.cap_id = 1U;
  service_cap.rights_mask = CAP_RIGHT_IPC_ENDPOINT;
  KPRINT("P2: capability granted id=1 rights=ipc\n");

  request.msg_id = 1U;
  request.msg_len = 6U;
  request.payload[0] = 'h';
  request.payload[1] = 'e';
  request.payload[2] = 'l';
  request.payload[3] = 'l';
  request.payload[4] = 'o';
  request.payload[5] = '\0';

  KPRINT("P2: ipc request sent\n");
  hello_service_task(&service_cap, &request, &reply);

  if (reply.msg_len > 0U && reply.payload[0] != '\0') {
    KPRINT("P2: ipc reply received payload=");
    hal_serial_write(reply.payload);
    KPRINT("\n");
    KPRINT("P2: hello ipc smoke test passed\n");
  } else {
    KPRINT("P2: hello ipc smoke test failed\n");
  }
}

// Basic entry point for the microkernel
void kernel_main(void) {
  const char *profile = kernel_boot_hw_profile();

  KPRINT("\n");
  KPRINT("  ██████╗ ██╗  ██╗ █████╗ ██████╗  █████╗ ████████╗\n");
  KPRINT("  ██╔══██╗██║  ██║██╔══██╗██╔══██╗██╔══██╗╚══██╔══╝\n");
  KPRINT("  ██████╔╝███████║███████║██████╔╝███████║   ██║\n");
  KPRINT("  ██╔══██╗██╔══██║██╔══██║██╔══██╗██╔══██║   ██║\n");
  KPRINT("  ██████╔╝██║  ██║██║  ██║██║  ██║██║  ██║   ██║\n");
  KPRINT("  ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝  ╚═╝\n");
  KPRINT("\n");
#if defined(__x86_64__) || defined(__i386__)
  KPRINT("  Bharat-OS  v0.1-dev  (x86_64 bring-up)\n");
#elif defined(__riscv)
  KPRINT("  Bharat-OS  v0.1-dev  (riscv64 bring-up)\n");
#elif defined(__aarch64__)
  KPRINT("  Bharat-OS  v0.1-dev  (arm64 bring-up)\n");
#else
  KPRINT("  Bharat-OS  v0.1-dev  (unknown arch bring-up)\n");
#endif
  KPRINT("  Verification-first microkernel — made in India\n");
  KPRINT("\n");
  KPRINT("  [HAL] Initialising hardware...\n");

  // 1. Initialize hardware architecture (CPU)
  hal_init();

  KPRINT("  [HAL] Ready.\n");

  KPRINT("  [MM]  Initializing memory...\n");
  // Proper memory map initialization will be passed from the bootloader.
  // We emit output to demonstrate the bring-up phase.
  KPRINT("  [MM]  Physical memory manager scaffolding initialized.\n");

  KPRINT("  [CPU] Enabling interrupts...\n");
  hal_cpu_enable_interrupts();
  KPRINT("  [CPU] Interrupts enabled.\n");

  kernel_phase2_hello_service_smoke();

  KPRINT("  [MK]  Entering halt loop (no scheduler yet).\n");
  KPRINT("\n");

  hal_serial_write("[bharat] kernel_main reached\n");
  hal_serial_write("[bharat] hw_profile=");
  hal_serial_write(profile);
  hal_serial_write("\n");
#if BHARAT_BOOT_GUI
  hal_serial_write("[bharat] boot_gui=ON\n");
#else
  hal_serial_write("[bharat] boot_gui=OFF\n");
#endif

  while (1) {
    hal_cpu_halt();
  }
}
