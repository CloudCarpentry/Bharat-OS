#include "trap.h"
#ifdef BHARAT_PERSONALITY_LINUX
#include "../../personalities/linux/linux_compat.h"
extern long linux_syscall_handler(long sysno, long arg1, long arg2, long arg3,
                                  long arg4, long arg5, long arg6)
    __attribute__((weak));
#endif
#include "capability.h"
#include "device.h"
#include "hal/hal.h"
#include "hal/hal_irq.h"
#include "ipc_endpoint.h"
#include "kernel.h"
#include "kernel_safety.h"
#include "mm.h"
#include "mm/mm_local.h"
#include "fault_diag.h"
#include "arch/arch_ext_state.h"
#include "arch/arch_caps.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
int vmm_handle_cow_fault(address_space_t* as, virt_addr_t vaddr);
extern bool core_is_rt(void);

#define TRAP_SUCCESS 0L
#define TRAP_ERR_INVAL (-22L)
#define TRAP_ERR_PERM (-1L)
#define TRAP_ERR_NOSYS (-38L)

#if defined(__x86_64__)
#define TRAP_CAUSE_SYSCALL 0x80U
#elif defined(__riscv)
#define TRAP_CAUSE_SYSCALL 8U
#else
#define TRAP_CAUSE_SYSCALL 0xFFFFU
#endif

static kprocess_t g_syscall_proc;

// Architectural guardrail: The `g_syscall_proc` singleton is a bootstrap fallback only.
// In steady state, syscalls and faults must resolve authority against the current thread/process.
// Cross-core URPC processing in the trap return path is strictly bounded and local.

static capability_table_t *trap_current_cap_table(void) {
  capability_table_t *table = sched_current_cap_table();
  if (table) {
    return table;
  }
  // Fallback for early bootstrap when no user context exists
  return (capability_table_t *)g_syscall_proc.security_sandbox_ctx;
}

static kprocess_t *trap_current_process(void) {
  kprocess_t *proc = sched_current_process();
  if (proc) {
    return proc;
  }
  return &g_syscall_proc;
}

static address_space_t *trap_current_aspace(void) {
  address_space_t *as = sched_current_aspace();
  if (as) {
    return as;
  }
  return g_syscall_proc.addr_space;
}

static void trap_device_irq_dispatch(uint32_t irq, void* ctx) {
  (void)ctx;
  device_dispatch_irq(irq);
}

static int trap_user_ptr_valid(uint64_t ptr) {
  const uint64_t USER_MIN = 0x1000ULL;
  uint64_t USER_MAX = 0x7FFFFFFFULL; // Compact 32-bit layout

  if (arch_has_cap(ARCH_CAP_USERSPACE_HIGHHALF) && arch_has_cap(ARCH_CAP_64BIT_VA)) {
      USER_MAX = 0x00007FFFFFFFFFFFULL;
  }

  return BHARAT_RANGE_VALID(ptr, USER_MIN, USER_MAX);
}

static int trap_user_range_valid(uint64_t ptr, uint64_t len) {
  if (len == 0U) {
    return 1;
  }

  if (len > (uint64_t)SIZE_MAX) {
    return 0;
  }

  if (!trap_user_ptr_valid(ptr)) {
    return 0;
  }

  uint64_t end_inclusive = ptr + len - 1U;
  if (end_inclusive < ptr) {
    return 0;
  }

  return trap_user_ptr_valid(end_inclusive);
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
    if (!trap_user_range_valid(arg1, (uint64_t)sizeof(*out_tid))) {
      return TRAP_ERR_INVAL;
    }
    return (long)sched_sys_thread_create(
        trap_current_process(), (void (*)(void))(uintptr_t)arg0, out_tid);
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
    if (!trap_user_range_valid(arg0, (uint64_t)sizeof(*out_send_cap)) ||
        !trap_user_range_valid(arg1, (uint64_t)sizeof(*out_recv_cap))) {
      return TRAP_ERR_INVAL;
    }
    return (long)ipc_endpoint_create(table, out_send_cap, out_recv_cap);
  }
  case SYSCALL_ENDPOINT_SEND: {
    capability_table_t *table = trap_current_cap_table();
    if (!trap_user_range_valid(arg1, arg2)) {
      return TRAP_ERR_INVAL;
    }
    return (long)ipc_endpoint_send(
        table, (uint32_t)arg0, (const void *)(uintptr_t)arg1, (uint32_t)arg2, (uint64_t)arg3, (uint32_t)arg4, (uint32_t)arg5);
  }
  case SYSCALL_ENDPOINT_RECEIVE: {
    capability_table_t *table = trap_current_cap_table();
    uint32_t *out_len = (uint32_t *)(uintptr_t)arg3;
    uint32_t *out_received_cap = (uint32_t *)(uintptr_t)arg5;
    if (!trap_user_range_valid(arg1, arg2) ||
        !trap_user_range_valid(arg3, (uint64_t)sizeof(*out_len)) ||
        (out_received_cap && !trap_user_range_valid(arg5, (uint64_t)sizeof(*out_received_cap)))) {
      return TRAP_ERR_INVAL;
    }
    return (long)ipc_endpoint_receive(table, (uint32_t)arg0,
                                      (void *)(uintptr_t)arg1, (uint32_t)arg2,
                                      out_len, (uint64_t)arg4, out_received_cap);
  }
  case SYSCALL_CAPABILITY_DELEGATE: {
    capability_table_t *table = trap_current_cap_table();
    uint32_t *out_cap = (uint32_t *)(uintptr_t)arg2;
    if (!trap_user_range_valid(arg2, (uint64_t)sizeof(*out_cap))) {
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

  if (frame->type == TRAP_TYPE_IRQ) {
    void hal_timer_isr(void);
    hal_interrupt_handle_trap_irq(frame->cause, hal_timer_isr,
                                  trap_device_irq_dispatch, NULL);
    return 0;
  }

  if (frame->type == TRAP_TYPE_SYNC || frame->type == TRAP_TYPE_SERROR) {
    kthread_t *current = sched_current_thread();

    if (hal_cpu_is_fp_simd_fault(frame)) {
      if (arch_ext_state_handle_fault(current)) {
        return 0; // retry
      }
    }

    if (hal_cpu_is_page_fault(frame)) {
      uint64_t fault_addr = hal_cpu_get_fault_address(frame);
      fault_diag_record_fault(fault_addr, frame->cause);

      // Check if it's a guard page hit
      if (current && current->kernel_stack != 0 && fault_addr >= current->kernel_stack && fault_addr < current->kernel_stack + PAGE_SIZE) {
          if (core_is_rt()) {
              panic_context_t pctx = {
                .message = "Stack overflow on RT core! Guard page hit.",
                .cause_str = "stack_overflow/guard_page",
                .cause_code = frame->cause,
                .fault_addr = fault_addr,
                .ip = frame->pc,
                .sp = frame->sp,
                .trap_frame = frame
              };
              kernel_panic_ex(&pctx);
          } else {
              thread_raise_fault(current, THREAD_FAULT_STACK_OVERFLOW);
              return 0;
          }
      }

      if (current && current->process_id != 0) {
        // Address space from current process
        // For proper hardware demand-paging, we call into the VMM.
        int rc = vmm_handle_cow_fault(trap_current_aspace(), fault_addr);
        if (rc == 0) {
            return 0;
        }
      }
    }
  }

  if (frame->cause != TRAP_CAUSE_SYSCALL) {
    if (frame->from_user == 0U) {
      panic_context_t pctx = {
          .message = "Kernel exception",
          .cause_str = "unhandled_kernel_trap",
          .cause_code = frame->cause,
          .ip = frame->pc,
          .sp = frame->sp,
          .trap_frame = frame
      };
      kernel_panic_ex(&pctx);
    } else {
      kthread_t *current = sched_current_thread();
      if (current) {
        thread_raise_fault(current, THREAD_FAULT_SEGV);
      } else {
        sched_yield();
      }
    }
    return 0; // Return gracefully after handling exception (if user thread)
  }

  // Before returning to user space, process any pending incoming URPC messages
  // like TLB shootdowns to ensure consistency.
  // We process only local messages for the current core to maintain multikernel
  // architecture separation.
  extern void vmm_process_local_urpc_messages(uint32_t core_id);
  vmm_process_local_urpc_messages(hal_cpu_get_id());

  if (frame->from_user == 0U) {
    return TRAP_ERR_PERM;
  }

  fault_diag_record_syscall(frame->gpr[0]);

  kthread_t *current = sched_current_thread();
  long rc = 0;

  if (current && current->personality == PERSONALITY_LINUX) {
#ifdef BHARAT_PERSONALITY_LINUX
    if (linux_syscall_handler) {
      rc = linux_syscall_handler(frame->gpr[0], frame->gpr[1], frame->gpr[2],
                                 frame->gpr[3], frame->gpr[4], frame->gpr[5],
                                 frame->gpr[6]);
    } else {
      rc = TRAP_ERR_NOSYS;
    }
#else
    rc = TRAP_ERR_NOSYS;
#endif
  } else {
    rc = syscall_dispatch((syscall_id_t)frame->gpr[0], frame->gpr[1],
                          frame->gpr[2], frame->gpr[3], frame->gpr[4],
                          frame->gpr[5], frame->gpr[6]);
  }

  frame->gpr[0] = (uint64_t)rc;

  return rc;
}
