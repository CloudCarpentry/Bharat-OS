#include "trap.h"
#include "personality_ops.h"
#include "capability.h"
#include "device.h"
#include "hal/hal.h"
#include "hal/hal_irq.h"
#include "ipc_endpoint.h"
#include "kernel.h"
#include "kernel/status.h"
#include "kernel_safety.h"
#include "mm.h"
#include "mm/mm_local.h"
#include "fault_diag.h"
#include <bharat/uapi/syscall_args.h>
#include "arch/arch_ext_state.h"
#include "arch/arch_caps.h"
#include "trap_api.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
int vmm_handle_cow_fault(address_space_t* as, virt_addr_t vaddr);
extern bool core_is_rt(void);

#define TRAP_SUCCESS 0L
#define TRAP_ERR_INVAL (-(long)SYS_EINVAL)
#define TRAP_ERR_PERM (-(long)SYS_EPERM)
#define TRAP_ERR_NOSYS (-(long)SYS_ENOSYS)


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

static bool trap_is_known_sys_errno(long err) {
  switch (err) {
  case SYS_EPERM:
  case SYS_ENOENT:
  case SYS_ESRCH:
  case SYS_EINTR:
  case SYS_EIO:
  case SYS_ENXIO:
  case SYS_EBADF:
  case SYS_EAGAIN:
  case SYS_ENOMEM:
  case SYS_EACCES:
  case SYS_EFAULT:
  case SYS_EBUSY:
  case SYS_EEXIST:
  case SYS_ENODEV:
  case SYS_ENOTDIR:
  case SYS_EISDIR:
  case SYS_EINVAL:
  case SYS_ENOSPC:
  case SYS_EROFS:
  case SYS_EPIPE:
  case SYS_ENOSYS:
  case SYS_EADDRNOTAVAIL:
  case SYS_ENETDOWN:
  case SYS_ENETUNREACH:
  case SYS_ECONNRESET:
  case SYS_ETIMEDOUT:
  case SYS_ECONNREFUSED:
  case SYS_EHOSTUNREACH:
    return true;
  default:
    return false;
  }
}

static long trap_status_to_sysret(long rc, sys_errno_t legacy_fallback) {
  if (rc == 0) {
    return TRAP_SUCCESS;
  }

  if (rc > 0) {
    return rc;
  }

  const long err = -rc;
  if (trap_is_known_sys_errno(err)) {
    return rc;
  }

  /* Rich kernel statuses have stable K_ERR families. */
  if ((rc <= -256) || (rc >= -17 && rc <= -1)) {
    return kstatus_to_sysret((kstatus_t)rc);
  }

  /* Legacy subsystem-local negatives collapse to deterministic errno. */
  return -(long)legacy_fallback;
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

extern const personality_ops_t default_personality_ops;

int trap_init(void) {
  g_syscall_proc.process_id = 0U;
  g_syscall_proc.addr_space = mm_create_address_space();
  g_syscall_proc.main_thread = NULL;
  g_syscall_proc.security_sandbox_ctx = NULL;
  g_syscall_proc.personality_ops = &default_personality_ops;

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
#define SYSCALL_ARGS_FROM_USER(type_, user_ptr_)   ({     const type_ *__p = (const type_ *)(uintptr_t)(user_ptr_);     if (!trap_user_range_valid((uint64_t)(user_ptr_), (uint64_t)sizeof(type_))) {       return TRAP_ERR_INVAL;     }     __p;   })
#define SYSCALL_RET_FROM_STATUS(expr_, fallback_) trap_status_to_sysret((long)(expr_), (fallback_))

  switch (id) {
  case SYSCALL_NOP:
    return TRAP_SUCCESS;
  case SYSCALL_THREAD_CREATE: {
    uint64_t *out_tid = (uint64_t *)(uintptr_t)arg1;
    if (!trap_user_range_valid(arg1, (uint64_t)sizeof(*out_tid))) {
      return TRAP_ERR_INVAL;
    }
    return SYSCALL_RET_FROM_STATUS(
        sched_sys_thread_create(trap_current_process(), (void (*)(void))(uintptr_t)arg0, out_tid),
        SYS_ENOMEM);
  }
  case SYSCALL_THREAD_DESTROY:
    return SYSCALL_RET_FROM_STATUS(sched_sys_thread_destroy(arg0), SYS_ENOENT);
  case SYSCALL_SCHED_YIELD:
    sched_yield();
    return TRAP_SUCCESS;
  case SYSCALL_SCHED_SLEEP:
    return SYSCALL_RET_FROM_STATUS(sched_sys_sleep(arg0), SYS_EIO);
  case SYSCALL_SCHED_SET_PRIORITY:
    return SYSCALL_RET_FROM_STATUS(sched_sys_set_priority(arg0, (uint32_t)arg1), SYS_EINVAL);
  case SYSCALL_SCHED_SET_AFFINITY:
    return SYSCALL_RET_FROM_STATUS(sched_sys_set_affinity(arg0, (uint32_t)arg1), SYS_EINVAL);
  case SYSCALL_VMM_MAP_PAGE: {
    const bharat_sys_vmm_map_page_args_t *args =
        SYSCALL_ARGS_FROM_USER(bharat_sys_vmm_map_page_args_t, arg0);
    return SYSCALL_RET_FROM_STATUS(
        vmm_map_page((virt_addr_t)args->vaddr, (phys_addr_t)args->paddr, args->flags),
        SYS_EFAULT);
  }
  case SYSCALL_VMM_UNMAP_PAGE:
    return SYSCALL_RET_FROM_STATUS(vmm_unmap_page((virt_addr_t)arg0), SYS_EFAULT);
  case SYSCALL_CAPABILITY_INVOKE: {
    const bharat_sys_cap_invoke_args_t *args =
        SYSCALL_ARGS_FROM_USER(bharat_sys_cap_invoke_args_t, arg0);
    return SYSCALL_RET_FROM_STATUS(cap_invoke(args->cap_id, args->opcode, args->arg0, args->arg1), SYS_EPERM);
  }
  case SYSCALL_ENDPOINT_CREATE: {
    capability_table_t *table = trap_current_cap_table();
    const bharat_sys_endpoint_create_args_t *args =
        SYSCALL_ARGS_FROM_USER(bharat_sys_endpoint_create_args_t, arg0);
    uint32_t *out_send_cap = (uint32_t *)(uintptr_t)args->out_send_cap_ptr;
    uint32_t *out_recv_cap = (uint32_t *)(uintptr_t)args->out_recv_cap_ptr;
    if (!trap_user_range_valid(args->out_send_cap_ptr, (uint64_t)sizeof(*out_send_cap)) ||
        !trap_user_range_valid(args->out_recv_cap_ptr, (uint64_t)sizeof(*out_recv_cap))) {
      return TRAP_ERR_INVAL;
    }
    return SYSCALL_RET_FROM_STATUS(ipc_endpoint_create(table, out_send_cap, out_recv_cap), SYS_ENOSPC);
  }
  case SYSCALL_ENDPOINT_SEND: {
    capability_table_t *table = trap_current_cap_table();
    const bharat_sys_endpoint_send_args_t *args =
        SYSCALL_ARGS_FROM_USER(bharat_sys_endpoint_send_args_t, arg0);
    if (!trap_user_range_valid(args->payload_ptr, args->payload_len)) {
      return TRAP_ERR_INVAL;
    }
    return SYSCALL_RET_FROM_STATUS(
        ipc_endpoint_send(table, args->send_cap, (const void *)(uintptr_t)args->payload_ptr,
                          args->payload_len, args->timeout_ticks, args->cap_to_send, args->cap_send_rights),
        SYS_EIO);
  }
  case SYSCALL_ENDPOINT_RECEIVE: {
    capability_table_t *table = trap_current_cap_table();
    const bharat_sys_endpoint_receive_args_t *args =
        SYSCALL_ARGS_FROM_USER(bharat_sys_endpoint_receive_args_t, arg0);
    uint32_t *out_len = (uint32_t *)(uintptr_t)args->out_len_ptr;
    uint32_t *out_received_cap = (uint32_t *)(uintptr_t)args->out_received_cap_ptr;
    if (!trap_user_range_valid(args->out_payload_ptr, args->out_payload_capacity) ||
        !trap_user_range_valid(args->out_len_ptr, (uint64_t)sizeof(*out_len)) ||
        (out_received_cap && !trap_user_range_valid(args->out_received_cap_ptr, (uint64_t)sizeof(*out_received_cap)))) {
      return TRAP_ERR_INVAL;
    }
    return SYSCALL_RET_FROM_STATUS(
        ipc_endpoint_receive(table, args->recv_cap, (void *)(uintptr_t)args->out_payload_ptr,
                             args->out_payload_capacity, out_len, args->timeout_ticks,
                             out_received_cap),
        SYS_EIO);
  }
  case SYSCALL_CAPABILITY_DELEGATE: {
    capability_table_t *table = trap_current_cap_table();
    const bharat_sys_cap_delegate_args_t *args =
        SYSCALL_ARGS_FROM_USER(bharat_sys_cap_delegate_args_t, arg0);
    uint32_t *out_cap = (uint32_t *)(uintptr_t)args->out_cap_ptr;
    if (!trap_user_range_valid(args->out_cap_ptr, (uint64_t)sizeof(*out_cap))) {
      return TRAP_ERR_INVAL;
    }
    return SYSCALL_RET_FROM_STATUS(
        cap_table_delegate(table, table, args->src_cap, args->requested_rights, out_cap),
        SYS_EPERM);
  }
  default:
    return TRAP_ERR_NOSYS;
  }

#undef SYSCALL_ARGS_FROM_USER
#undef SYSCALL_RET_FROM_STATUS
}

int trap_dispatch(trap_frame_t *frame, const trap_info_t *info) {
  if (!frame || !info) {
    return TRAP_ERR_INVAL;
  }

  // Common cross-core URPC processing before returning to user space
  // We process only local messages for the current core to maintain multikernel
  // architecture separation.
  extern void vmm_process_local_urpc_messages(uint32_t core_id);
  vmm_process_local_urpc_messages(hal_cpu_get_id());

  kthread_t *current = sched_current_thread();

  switch (info->trap_class) {
  case TRAP_CLASS_INTERRUPT:
  case TRAP_CLASS_TIMER:
  case TRAP_CLASS_IPI: {
    void hal_timer_isr(void);
    hal_interrupt_handle_trap_irq(info->arch_code, hal_timer_isr,
                                  trap_device_irq_dispatch, NULL);

    if (info->trap_class == TRAP_CLASS_IPI) {
        // IPI could be a remote enqueue notification. We call sched_reschedule
        // to drain the remote inbox and possibly preempt the current thread.
        sched_reschedule();
    }
    return 0;
  }
  case TRAP_CLASS_SYSCALL: {
    if (info->origin == TRAP_ORIGIN_KERNEL) {
      return TRAP_ERR_PERM;
    }

    // Instead of directly running syscall_dispatch, we delegate it via our generic handler
    long rc = trap_dispatch_syscall(frame, info);

    return (int)rc;
  }
  case TRAP_CLASS_PAGE_FAULT:
  case TRAP_CLASS_ACCESS_FAULT: {
    fault_diag_record_fault(info->fault_addr, info->arch_code);

    // Check if it's a guard page hit
    if (current && current->kernel_stack != 0 && info->fault_addr >= current->kernel_stack && info->fault_addr < current->kernel_stack + PAGE_SIZE) {
        if (core_is_rt()) {
            panic_context_t pctx = {
              .message = "Stack overflow on RT core! Guard page hit.",
              .cause_str = "stack_overflow/guard_page",
              .cause_code = info->arch_code,
              .fault_addr = info->fault_addr,
              .ip = info->ip,
              .sp = info->sp,
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
      int rc = vmm_handle_cow_fault(trap_current_aspace(), info->fault_addr);
      if (rc == 0) {
          return 0;
      }
    }

    // If we could not resolve the page fault, fall back to handle_fault
    return trap_handle_fault(frame, info);
  }
  case TRAP_CLASS_ILLEGAL_INSTR:
  case TRAP_CLASS_ALIGNMENT:
  case TRAP_CLASS_BREAKPOINT:
  case TRAP_CLASS_GENERAL_FAULT:
  case TRAP_CLASS_UNKNOWN:
  default:
    // Try to resolve FPU fault if applicable, although ideally arch decode would have separated FP disabled exceptions.
    // For now, keep the generic helper around if needed:
    if (hal_cpu_is_fp_simd_fault(frame)) {
      if (arch_ext_state_handle_fault(current)) {
        return 0; // retry
      }
    }
    return trap_handle_fault(frame, info);
  }
}

// Temporary shim to keep current tests/arch building, to be replaced by full arch decode!
long trap_handle(trap_frame_t *frame) {
  if (!frame) return TRAP_ERR_INVAL;

  trap_info_t info = {0};
  info.origin = frame->from_user ? TRAP_ORIGIN_USER : TRAP_ORIGIN_KERNEL;
  info.ip = frame->pc;
  info.sp = frame->sp;
  info.arch_code = frame->cause;

  if (frame->type == TRAP_TYPE_IRQ) {
    info.trap_class = TRAP_CLASS_INTERRUPT;
  } else {
#if defined(__x86_64__)
    uint64_t trap_cause_syscall = 0x80U;
#elif defined(__riscv)
    uint64_t trap_cause_syscall = 8U;
#else
    uint64_t trap_cause_syscall = 0xFFFFU;
#endif

    if (frame->cause == trap_cause_syscall) {
      info.trap_class = TRAP_CLASS_SYSCALL;
    } else if (hal_cpu_is_page_fault(frame)) {
      info.trap_class = TRAP_CLASS_PAGE_FAULT;
      info.fault_addr = hal_cpu_get_fault_address(frame);
    } else {
      info.trap_class = TRAP_CLASS_GENERAL_FAULT;
    }
  }

  return trap_dispatch(frame, &info);
}
