#include <stdint.h>
#include <stddef.h>

int sched_sys_intent_set(uint64_t tid, const void* intent);
int sched_sys_intent_get(uint64_t tid, void* intent);
int sys_mem_alloc_class(size_t size, uint32_t mem_class, uint32_t flags, uint64_t* out_addr);
int sys_fault_domain_create(const void* attr, uint64_t* out_domain);
int sys_fault_domain_destroy(uint64_t domain);
int sys_fault_domain_attach(uint64_t domain, uint64_t tid);

#include <bharat/uapi/system/intent.h>
#include <bharat/uapi/system/fault_domain.h>
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
#define TRAP_ERR_INVAL kstatus_to_sysret(K_ERR_INVALID_ARG)
#define TRAP_ERR_PERM kstatus_to_sysret(K_ERR_DENIED)
#define TRAP_ERR_NOSYS kstatus_to_sysret(K_ERR_UNSUPPORTED)
#define TRAP_ERR_FAULT kstatus_to_sysret(K_ERR_FAULT)

static bh_process_t g_syscall_proc;

static capability_table_t *trap_current_cap_table(void) {
  capability_table_t *table = sched_current_cap_table();
  if (table) {
    return table;
  }
  return (capability_table_t *)g_syscall_proc.security_sandbox_ctx;
}

static bh_process_t *trap_current_process(void) {
  bh_process_t *proc = sched_current_process();
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

static kstatus_t ipc_status_to_kstatus(int ipc_status) {
    switch (ipc_status) {
        case IPC_OK: return K_OK;
        case IPC_ERR_INVALID: return K_ERR_INVALID_ARG;
        case IPC_ERR_NO_SPACE: return K_ERR_NO_MEMORY;
        case IPC_ERR_WOULD_BLOCK: return K_ERR_AGAIN;
        case IPC_ERR_PERM: return K_ERR_DENIED;
        case IPC_ERR_CAP_TRANSFER_NOT_ALLOWED: return K_ERR_CAP_DENIED;
        case IPC_ERR_CAP_INSTALL_FAILED: return K_ERR_INTERNAL_BUG;
        case IPC_ERR_CLOSED: return K_ERR_BAD_STATE;
        case IPC_ERR_TIMEOUT: return K_ERR_TIMEOUT;
        default: return K_ERR_INTERNAL_BUG;
    }
}

int cap_invoke(uintptr_t cap_id, uintptr_t opcode, uintptr_t arg0, uintptr_t arg1)
    __attribute__((weak));
int cap_invoke(uintptr_t cap_id, uintptr_t opcode, uintptr_t arg0, uintptr_t arg1) {
  (void)cap_id;
  (void)opcode;
  (void)arg0;
  (void)arg1;
  return -1;
}

#include "personality/personality_hooks.h"

int trap_init(void) {
  g_syscall_proc.process_id = 0U;
  g_syscall_proc.addr_space = mm_create_address_space();
  g_syscall_proc.main_thread = NULL;
  g_syscall_proc.security_sandbox_ctx = NULL;

  g_syscall_proc.personality_ops = personality_get_current_ops();

  if (!g_syscall_proc.addr_space) {
    return -1;
  }

  if (cap_table_init_for_process(&g_syscall_proc) != 0) {
    return -1;
  }

  return 0;
}


int trap_dispatch(trap_frame_t *frame, const trap_info_t *info) {
  if (!frame || !info) {
    return (int)TRAP_ERR_INVAL;
  }

  extern void vmm_process_local_urpc_messages(uint32_t core_id);
  vmm_process_local_urpc_messages(hal_cpu_get_id());

  bh_thread_t *current = sched_current_thread();

  switch (info->trap_class) {
  case TRAP_CLASS_INTERRUPT:
  case TRAP_CLASS_TIMER:
  case TRAP_CLASS_IPI: {
    void hal_timer_isr(void);
    hal_interrupt_handle_trap_irq(info->arch_code, hal_timer_isr,
                                  trap_device_irq_dispatch, NULL);

    if (info->trap_class == TRAP_CLASS_IPI) {
        bh_thread_yield();
    }
    return 0;
  }
  case TRAP_CLASS_SYSCALL: {
    if (info->origin == TRAP_ORIGIN_KERNEL) {
      return (int)TRAP_ERR_PERM;
    }

    long rc = trap_dispatch_syscall(frame, info);

    return (int)rc;
  }
  case TRAP_CLASS_PAGE_FAULT:
  case TRAP_CLASS_ACCESS_FAULT: {
    fault_diag_record_fault(info->fault_addr, info->arch_code);

    if (current && current->kernel_stack != 0 && info->fault_addr >= current->kernel_stack && info->fault_addr < current->kernel_stack + PAGE_SIZE) {
        if (core_is_rt()) {
            panic_context_t pctx = {
              .message = "Stack overflow on RT core! Guard page hit.",
              .cause_str = "stack_overflow/guard_page",
              .cause_code = info->arch_code,
              .fault_addr = (uint64_t)info->fault_addr,
              .ip = (uint64_t)info->ip,
              .sp = (uint64_t)info->sp,
              .trap_frame = frame
            };
            kernel_panic_ex(&pctx);
        } else {
            thread_raise_fault(current, THREAD_FAULT_STACK_OVERFLOW);
            return 0;
        }
    }

    if (current && current->process_id != 0) {
      int rc = vmm_handle_cow_fault(trap_current_aspace(), info->fault_addr);
      if (rc == 0) {
          return 0;
      }
    }

    return trap_handle_fault(frame, info);
  }
  case TRAP_CLASS_ILLEGAL_INSTR:
  case TRAP_CLASS_ALIGNMENT:
  case TRAP_CLASS_BREAKPOINT:
  case TRAP_CLASS_GENERAL_FAULT:
  case TRAP_CLASS_UNKNOWN:
  default:
    if (hal_cpu_is_fp_simd_fault(frame)) {
      if (arch_ext_state_handle_fault(current)) {
        return 0; // retry
      }
    }
    return trap_handle_fault(frame, info);
  }
}

#include "trap/syscall_regs.h"

long trap_handle(trap_frame_t *frame) {
  if (!frame) return TRAP_ERR_INVAL;

  trap_info_t info = {0};
  info.origin = frame->from_user ? TRAP_ORIGIN_USER : TRAP_ORIGIN_KERNEL;
  info.ip = (virt_addr_t)frame->pc;
  info.sp = (virt_addr_t)frame->sp;
  info.arch_code = (uint32_t)frame->cause;

  if (frame->type == TRAP_TYPE_IRQ) {
    info.trap_class = TRAP_CLASS_INTERRUPT;
  } else {
    if (arch_trap_is_syscall(frame)) {
      info.trap_class = TRAP_CLASS_SYSCALL;
    } else if (hal_cpu_is_page_fault(frame)) {
      info.trap_class = TRAP_CLASS_PAGE_FAULT;
      info.fault_addr = (virt_addr_t)hal_cpu_get_fault_address(frame);
    } else if (hal_cpu_is_access_fault(frame)) {
      info.trap_class = TRAP_CLASS_ACCESS_FAULT;
      info.fault_addr = (virt_addr_t)hal_cpu_get_fault_address(frame);
    } else {
      info.trap_class = TRAP_CLASS_GENERAL_FAULT;
    }
  }

  return (long)trap_dispatch(frame, &info);
}
// Needs updating with the new cases.
