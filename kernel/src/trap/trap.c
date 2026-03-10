#include "trap.h"
#include "../../subsys/include/linux_compat.h"
#include "capability.h"
#include "device.h"
#include "hal/hal.h"
#include "ipc_endpoint.h"
#include "kernel.h"
#include "kernel_safety.h"
#include "mm.h"


#include <stddef.h>
#include <stdint.h>

#define TRAP_SUCCESS 0L
#define TRAP_ERR_INVAL (-22L)
#define TRAP_ERR_PERM (-1L)
#define TRAP_ERR_NOSYS (-38L)

#if defined(__x86_64__)
#define TRAP_CAUSE_SYSCALL 0x80U
#define TRAP_CAUSE_TIMER_INT 32U
#elif defined(__riscv)
#define TRAP_CAUSE_SYSCALL 8U
#define TRAP_CAUSE_TIMER_INT 0x8000000000000005ULL // Supervisor timer interrupt
#else
#define TRAP_CAUSE_SYSCALL 0xFFFFU
#define TRAP_CAUSE_TIMER_INT 30U // Generic timer PPI on ARM
#endif

static kprocess_t g_syscall_proc;

static capability_table_t *trap_current_cap_table(void) {
  return (capability_table_t *)g_syscall_proc.security_sandbox_ctx;
}

static int trap_user_ptr_valid(uint64_t ptr) {
  const uint64_t USER_MIN = 0x1000ULL;
  const uint64_t USER_MAX = 0x00007FFFFFFFFFFFULL;
  return BHARAT_RANGE_VALID(ptr, USER_MIN, USER_MAX);
}

int cap_invoke(uint64_t cap_id, uint64_t opcode, uint64_t arg0, uint64_t arg1)
    __attribute__((weak));
int cap_invoke(uint64_t cap_id, uint64_t opcode, uint64_t arg0, uint64_t arg1) {
  (void)cap_id;
  (void)opcode;
  (void)arg0;
  (void)arg1;
  return -1;
}

int trap_init(void) {
  g_syscall_proc.process_id = 0U;
  g_syscall_proc.addr_space = mm_create_address_space();
  g_syscall_proc.main_thread = NULL;
  g_syscall_proc.security_sandbox_ctx = NULL;

  if (!g_syscall_proc.addr_space) {
    return -1;
  }

  if (cap_table_init_for_process(&g_syscall_proc) != 0) {
    return -1;
  }

  return 0;
}

long syscall_dispatch(syscall_id_t id, uint64_t arg0, uint64_t arg1,
                      uint64_t arg2, uint64_t arg3, uint64_t arg4,
                      uint64_t arg5) {
  (void)arg4;
  (void)arg5;

  switch (id) {
  case SYSCALL_NOP:
    return TRAP_SUCCESS;
  case SYSCALL_THREAD_CREATE: {
    uint64_t *out_tid = (uint64_t *)(uintptr_t)arg1;
    if (!trap_user_ptr_valid(arg1)) {
      return TRAP_ERR_INVAL;
    }
    return (long)sched_sys_thread_create(
        &g_syscall_proc, (void (*)(void))(uintptr_t)arg0, out_tid);
  }
  case SYSCALL_THREAD_DESTROY:
    return (long)sched_sys_thread_destroy(arg0);
  case SYSCALL_SCHED_YIELD:
    sched_yield();
    return TRAP_SUCCESS;
  case SYSCALL_SCHED_SLEEP:
    return (long)sched_sys_sleep(arg0);
  case SYSCALL_SCHED_SET_PRIORITY:
    return (long)sched_sys_set_priority(arg0, (uint32_t)arg1);
  case SYSCALL_SCHED_SET_AFFINITY:
    return (long)sched_sys_set_affinity(arg0, (uint32_t)arg1);
  case SYSCALL_VMM_MAP_PAGE:
    return (long)vmm_map_page((virt_addr_t)arg0, (phys_addr_t)arg1,
                              (uint32_t)arg2);
  case SYSCALL_VMM_UNMAP_PAGE:
    return (long)vmm_unmap_page((virt_addr_t)arg0);
  case SYSCALL_CAPABILITY_INVOKE:
    return (long)cap_invoke(arg0, arg1, arg2, arg3);
  case SYSCALL_ENDPOINT_CREATE: {
    capability_table_t *table = trap_current_cap_table();
    uint32_t *out_send_cap = (uint32_t *)(uintptr_t)arg0;
    uint32_t *out_recv_cap = (uint32_t *)(uintptr_t)arg1;
    if (!trap_user_ptr_valid(arg0) || !trap_user_ptr_valid(arg1)) {
      return TRAP_ERR_INVAL;
    }
    return (long)ipc_endpoint_create(table, out_send_cap, out_recv_cap);
  }
  case SYSCALL_ENDPOINT_SEND: {
    capability_table_t *table = trap_current_cap_table();
    if (!trap_user_ptr_valid(arg1)) {
      return TRAP_ERR_INVAL;
    }
    return (long)ipc_endpoint_send(
        table, (uint32_t)arg0, (const void *)(uintptr_t)arg1, (uint32_t)arg2);
  }
  case SYSCALL_ENDPOINT_RECEIVE: {
    capability_table_t *table = trap_current_cap_table();
    uint32_t *out_len = (uint32_t *)(uintptr_t)arg3;
    if (!trap_user_ptr_valid(arg1) || !trap_user_ptr_valid(arg3)) {
      return TRAP_ERR_INVAL;
    }
    return (long)ipc_endpoint_receive(table, (uint32_t)arg0,
                                      (void *)(uintptr_t)arg1, (uint32_t)arg2,
                                      out_len);
  }
  case SYSCALL_CAPABILITY_DELEGATE: {
    capability_table_t *table = trap_current_cap_table();
    uint32_t *out_cap = (uint32_t *)(uintptr_t)arg2;
    if (!trap_user_ptr_valid(arg2)) {
      return TRAP_ERR_INVAL;
    }
    return (long)cap_table_delegate(table, table, (uint32_t)arg0,
                                    (uint32_t)arg1, out_cap);
  }
  default:
    return TRAP_ERR_NOSYS;
  }
}

long trap_handle(trap_frame_t *frame) {
  if (!frame) {
    return TRAP_ERR_INVAL;
  }

#if defined(__riscv)
  if (frame->cause == TRAP_CAUSE_TIMER_INT) {
    void hal_timer_isr(void);
    hal_timer_isr();
    return 0;
  }
#elif defined(__x86_64__)
  if (frame->cause == TRAP_CAUSE_TIMER_INT) {
    void default_timer_isr(void);
    default_timer_isr();
    return 0;
  }
#else
  if (frame->type == TRAP_TYPE_IRQ) {
    uint32_t irq = hal_interrupt_acknowledge();
    if (irq == TRAP_CAUSE_TIMER_INT) {
      void hal_timer_isr(void);
      hal_timer_isr();
    } else {
      device_dispatch_irq(irq);
    }
    hal_interrupt_end_of_interrupt(irq);
    return 0;
  }
#endif

  if (frame->cause != TRAP_CAUSE_SYSCALL) {
    if (frame->from_user == 0U) {
      kernel_panic("Kernel exception");
    } else {
      kthread_t *current = sched_current_thread();
      if (current) {
        thread_destroy(current);
      }
      sched_yield();
    }
    return 0; // Return gracefully after handling exception (if user thread)
  }

  if (frame->from_user == 0U) {
    return TRAP_ERR_PERM;
  }

  kthread_t *current = sched_current_thread();
  long rc = 0;

  if (current && current->personality == PERSONALITY_LINUX) {
    rc = linux_syscall_handler(frame->gpr[0], frame->gpr[1], frame->gpr[2],
                               frame->gpr[3], frame->gpr[4], frame->gpr[5],
                               frame->gpr[6]);
  } else {
    rc = syscall_dispatch((syscall_id_t)frame->gpr[0], frame->gpr[1],
                          frame->gpr[2], frame->gpr[3], frame->gpr[4],
                          frame->gpr[5], frame->gpr[6]);
  }

  frame->gpr[0] = (uint64_t)rc;

  return rc;
}
