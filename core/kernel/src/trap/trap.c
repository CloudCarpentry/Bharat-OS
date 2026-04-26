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

static int trap_user_ptr_valid(uintptr_t ptr) {
  uintptr_t USER_MIN = (uintptr_t)0x1000;
  uintptr_t USER_MAX = (uintptr_t)0x7FFFFFFF; // Compact 32-bit layout

  if (arch_has_cap(ARCH_CAP_USERSPACE_HIGHHALF) && arch_has_cap(ARCH_CAP_64BIT_VA)) {
#if defined(__BHARAT_64BIT__) || defined(_WIN64) || defined(__LP64__) || defined(__x86_64__) || defined(__aarch64__) || defined(__riscv_xlen) && (__riscv_xlen == 64)
      USER_MAX = (uintptr_t)0x00007FFFFFFFFFFFULL;
#endif
  }

  return (ptr >= USER_MIN && ptr <= USER_MAX);
}

static int trap_user_range_valid(uintptr_t ptr, size_t len) {
  if (len == 0) {
    return 1;
  }

  if (!trap_user_ptr_valid(ptr)) {
    return 0;
  }

  uintptr_t end_inclusive = ptr + len - 1;
  if (end_inclusive < ptr) {
    return 0;
  }

  return trap_user_ptr_valid(end_inclusive);
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

long syscall_dispatch(syscall_id_t id, uintptr_t arg0, uintptr_t arg1,
                      uintptr_t arg2, uintptr_t arg3, uintptr_t arg4,
                      uintptr_t arg5) {
#define SYSCALL_ARGS_FROM_USER(type_, user_ptr_)   ({     const type_ *__p = (const type_ *)(uintptr_t)(user_ptr_);     if (!trap_user_range_valid((uintptr_t)(user_ptr_), (size_t)sizeof(type_))) {       return TRAP_ERR_FAULT;     }     __p;   })

  switch (id) {
  case SYSCALL_NOP:
    return TRAP_SUCCESS;
  case SYSCALL_THREAD_CREATE: {
    const bharat_sys_thread_create_args_t *args =
        SYSCALL_ARGS_FROM_USER(bharat_sys_thread_create_args_t, arg0);
    uint64_t *out_tid = (uint64_t *)(uintptr_t)args->out_tid_ptr;
    if (!trap_user_range_valid(args->out_tid_ptr, (size_t)sizeof(*out_tid))) {
      return TRAP_ERR_FAULT;
    }
    capability_entry_t e;
    if (cap_table_lookup(trap_current_cap_table(), args->process_cap, CAP_TYPE_PROCESS, CAP_RIGHT_PROCESS_MANAGE, &e) != 0) {
      return TRAP_ERR_PERM;
    }
    bh_process_t *target_proc = (bh_process_t *)e.object_ref;
    return kstatus_to_sysret(
        (kstatus_t)sched_sys_thread_create(target_proc, (void (*)(void))(uintptr_t)args->entry_point, out_tid));
  }
  case SYSCALL_THREAD_DESTROY: {
    capability_entry_t e;
    if (cap_table_lookup(trap_current_cap_table(), (uint32_t)arg0, CAP_TYPE_THREAD, CAP_RIGHT_REVOKE, &e) != 0) {
      return TRAP_ERR_PERM;
    }
    return kstatus_to_sysret((kstatus_t)sched_sys_thread_destroy((uint64_t)e.object_ref));
  }
  case SYSCALL_SCHED_YIELD:
    bh_thread_yield();
    return TRAP_SUCCESS;
  case SYSCALL_SCHED_SLEEP:
    return kstatus_to_sysret((kstatus_t)sched_sys_sleep((uint64_t)arg0));
  case SYSCALL_SCHED_SET_PRIORITY: {
    const bharat_sys_sched_attr_args_t *args =
        SYSCALL_ARGS_FROM_USER(bharat_sys_sched_attr_args_t, arg0);
    capability_entry_t e;
    if (cap_table_lookup(trap_current_cap_table(), args->thread_cap, CAP_TYPE_THREAD, CAP_RIGHT_SCHEDULE, &e) != 0) {
      return TRAP_ERR_PERM;
    }
    return kstatus_to_sysret((kstatus_t)sched_sys_set_priority((uint64_t)e.object_ref, (uint32_t)args->value));
  }
  case SYSCALL_SCHED_SET_AFFINITY: {
    const bharat_sys_sched_attr_args_t *args =
        SYSCALL_ARGS_FROM_USER(bharat_sys_sched_attr_args_t, arg0);
    capability_entry_t e;
    if (cap_table_lookup(trap_current_cap_table(), args->thread_cap, CAP_TYPE_THREAD, CAP_RIGHT_SCHEDULE, &e) != 0) {
      return TRAP_ERR_PERM;
    }
    return kstatus_to_sysret((kstatus_t)sched_sys_set_affinity((uint64_t)e.object_ref, (uint32_t)args->value));
  }
  case SYSCALL_VMM_MAP_PAGE: {
    const bharat_sys_vmm_map_page_args_t *args =
        SYSCALL_ARGS_FROM_USER(bharat_sys_vmm_map_page_args_t, arg0);
    if (!trap_user_ptr_valid(args->vaddr)) {
        return TRAP_ERR_FAULT;
    }
    capability_entry_t e;
    if (cap_table_lookup(trap_current_cap_table(), args->cap_id, CAP_TYPE_MEMORY, CAP_RIGHT_MEMORY_MAP, &e) != 0) {
      return TRAP_ERR_PERM;
    }
    return kstatus_to_sysret((kstatus_t)vmm_map_page((virt_addr_t)args->vaddr, (phys_addr_t)e.object_ref, args->flags));
  }
  case SYSCALL_VMM_UNMAP_PAGE: {
    const bharat_sys_vmm_unmap_page_args_t *args =
        SYSCALL_ARGS_FROM_USER(bharat_sys_vmm_unmap_page_args_t, arg0);
    if (!trap_user_ptr_valid(args->vaddr)) {
        return TRAP_ERR_FAULT;
    }
    capability_entry_t e;
    if (cap_table_lookup(trap_current_cap_table(), args->cap_id, CAP_TYPE_MEMORY, CAP_RIGHT_MEMORY_UNMAP, &e) != 0) {
      return TRAP_ERR_PERM;
    }
    return kstatus_to_sysret((kstatus_t)vmm_unmap_page((virt_addr_t)args->vaddr));
  }
  case SYSCALL_CAPABILITY_INVOKE: {
    const bharat_sys_cap_invoke_args_t *args =
        SYSCALL_ARGS_FROM_USER(bharat_sys_cap_invoke_args_t, arg0);
    return kstatus_to_sysret((kstatus_t)cap_invoke(args->cap_id, args->opcode, args->arg0, args->arg1));
  }
  case SYSCALL_ENDPOINT_CREATE: {
    capability_table_t *table = trap_current_cap_table();
    const bharat_sys_endpoint_create_args_t *args =
        SYSCALL_ARGS_FROM_USER(bharat_sys_endpoint_create_args_t, arg0);
    uint32_t *out_send_cap = (uint32_t *)(uintptr_t)args->out_send_cap_ptr;
    uint32_t *out_recv_cap = (uint32_t *)(uintptr_t)args->out_recv_cap_ptr;
    if (!trap_user_range_valid(args->out_send_cap_ptr, (size_t)sizeof(*out_send_cap)) ||
        !trap_user_range_valid(args->out_recv_cap_ptr, (size_t)sizeof(*out_recv_cap))) {
      return TRAP_ERR_FAULT;
    }
    return kstatus_to_sysret(ipc_status_to_kstatus(ipc_endpoint_create(table, out_send_cap, out_recv_cap)));
  }
  case SYSCALL_ENDPOINT_SEND: {
    capability_table_t *table = trap_current_cap_table();
    const bharat_sys_endpoint_send_args_t *args =
        SYSCALL_ARGS_FROM_USER(bharat_sys_endpoint_send_args_t, arg0);
    if (!trap_user_range_valid(args->payload_ptr, args->payload_len)) {
      return TRAP_ERR_FAULT;
    }
    return kstatus_to_sysret(
        ipc_status_to_kstatus(ipc_endpoint_send(table, args->send_cap, (const void *)(uintptr_t)args->payload_ptr,
                          args->payload_len, args->timeout_ticks, args->cap_to_send, args->cap_send_rights)));
  }
  case SYSCALL_ENDPOINT_RECEIVE: {
    capability_table_t *table = trap_current_cap_table();
    const bharat_sys_endpoint_receive_args_t *args =
        SYSCALL_ARGS_FROM_USER(bharat_sys_endpoint_receive_args_t, arg0);
    uint32_t *out_len = (uint32_t *)(uintptr_t)args->out_len_ptr;
    uint32_t *out_received_cap = (uint32_t *)(uintptr_t)args->out_received_cap_ptr;
    if (!trap_user_range_valid(args->out_payload_ptr, args->out_payload_capacity) ||
        !trap_user_range_valid(args->out_len_ptr, (size_t)sizeof(*out_len)) ||
        (out_received_cap && !trap_user_range_valid(args->out_received_cap_ptr, (size_t)sizeof(*out_received_cap)))) {
      return TRAP_ERR_FAULT;
    }
    return kstatus_to_sysret(
        ipc_status_to_kstatus(ipc_endpoint_receive(table, args->recv_cap, (void *)(uintptr_t)args->out_payload_ptr,
                             args->out_payload_capacity, out_len, args->timeout_ticks,
                             out_received_cap)));
  }
  case SYSCALL_CAPABILITY_DELEGATE: {
    capability_table_t *table = trap_current_cap_table();
    const bharat_sys_cap_delegate_args_t *args =
        SYSCALL_ARGS_FROM_USER(bharat_sys_cap_delegate_args_t, arg0);
    uint32_t *out_cap = (uint32_t *)(uintptr_t)args->out_cap_ptr;
    if (!trap_user_range_valid(args->out_cap_ptr, (size_t)sizeof(*out_cap))) {
      return TRAP_ERR_FAULT;
    }
    return kstatus_to_sysret(
        (kstatus_t)cap_table_delegate(table, table, args->src_cap, args->requested_rights, out_cap));
  }

  case SYSCALL_INTENT_SET: {
    const bharat_sys_intent_args_t *args =
        SYSCALL_ARGS_FROM_USER(bharat_sys_intent_args_t, arg0);
    capability_entry_t e;
    if (cap_table_lookup(trap_current_cap_table(), args->thread_cap, CAP_TYPE_THREAD, CAP_RIGHT_SCHEDULE, &e) != 0) {
      return TRAP_ERR_PERM;
    }
    if (!trap_user_range_valid(args->intent_ptr, (size_t)sizeof(bharat_intent_t))) {
      return TRAP_ERR_FAULT;
    }
    return kstatus_to_sysret((kstatus_t)sched_sys_intent_set((uint64_t)e.object_ref, (const void*)(uintptr_t)args->intent_ptr));
  }
  case SYSCALL_INTENT_GET: {
    const bharat_sys_intent_args_t *args =
        SYSCALL_ARGS_FROM_USER(bharat_sys_intent_args_t, arg0);
    capability_entry_t e;
    if (cap_table_lookup(trap_current_cap_table(), args->thread_cap, CAP_TYPE_THREAD, CAP_RIGHT_SCHEDULE, &e) != 0) {
      return TRAP_ERR_PERM;
    }
    if (!trap_user_range_valid(args->intent_ptr, (size_t)sizeof(bharat_intent_t))) {
      return TRAP_ERR_FAULT;
    }
    return kstatus_to_sysret((kstatus_t)sched_sys_intent_get((uint64_t)e.object_ref, (void*)(uintptr_t)args->intent_ptr));
  }
  case SYSCALL_MEM_ALLOC_CLASS: {
    const bharat_sys_mem_alloc_args_t *args =
        SYSCALL_ARGS_FROM_USER(bharat_sys_mem_alloc_args_t, arg0);
    uint64_t *out_addr = (uint64_t*)(uintptr_t)args->out_addr_ptr;
    if (!trap_user_range_valid(args->out_addr_ptr, (size_t)sizeof(*out_addr))) {
      return TRAP_ERR_FAULT;
    }
    capability_entry_t e;
    if (cap_table_lookup(trap_current_cap_table(), args->resource_cap, CAP_TYPE_PROCESS, CAP_RIGHT_RESOURCE_ALLOC, &e) != 0) {
      return TRAP_ERR_PERM;
    }
    return kstatus_to_sysret((kstatus_t)sys_mem_alloc_class((size_t)args->size, (uint32_t)args->mem_class, (uint32_t)args->flags, out_addr));
  }
  case SYSCALL_FAULT_DOMAIN_CREATE: {
    const bharat_sys_fault_domain_args_t *args =
        SYSCALL_ARGS_FROM_USER(bharat_sys_fault_domain_args_t, arg0);
    capability_entry_t e;
    if (cap_table_lookup(trap_current_cap_table(), args->cap_id, CAP_TYPE_PROCESS, CAP_RIGHT_FAULT_DOMAIN_MANAGE, &e) != 0) {
      return TRAP_ERR_PERM;
    }
    if (!trap_user_range_valid(args->attr_ptr, (size_t)sizeof(bharat_fault_domain_attr_t))) {
      return TRAP_ERR_FAULT;
    }
    uint64_t *out_domain = (uint64_t*)(uintptr_t)args->out_domain_ptr;
    if (!trap_user_range_valid(args->out_domain_ptr, (size_t)sizeof(*out_domain))) {
      return TRAP_ERR_FAULT;
    }
    return kstatus_to_sysret((kstatus_t)sys_fault_domain_create((const void*)(uintptr_t)args->attr_ptr, out_domain));
  }
  case SYSCALL_FAULT_DOMAIN_DESTROY: {
    capability_entry_t e;
    if (cap_table_lookup(trap_current_cap_table(), (uint32_t)arg0, CAP_TYPE_PROCESS, CAP_RIGHT_FAULT_DOMAIN_MANAGE, &e) != 0) {
      return TRAP_ERR_PERM;
    }
    return kstatus_to_sysret((kstatus_t)sys_fault_domain_destroy((uint64_t)e.object_ref));
  }
  case SYSCALL_FAULT_DOMAIN_ATTACH: {
    const bharat_sys_fault_domain_args_t *args =
        SYSCALL_ARGS_FROM_USER(bharat_sys_fault_domain_args_t, arg0);
    capability_entry_t e;
    if (cap_table_lookup(trap_current_cap_table(), args->cap_id, CAP_TYPE_PROCESS, CAP_RIGHT_FAULT_DOMAIN_MANAGE, &e) != 0) {
      return TRAP_ERR_PERM;
    }
    capability_entry_t te;
    if (cap_table_lookup(trap_current_cap_table(), args->thread_cap, CAP_TYPE_THREAD, CAP_RIGHT_SCHEDULE, &te) != 0) {
      return TRAP_ERR_PERM;
    }
    return kstatus_to_sysret((kstatus_t)sys_fault_domain_attach((uint64_t)e.object_ref, (uint64_t)te.object_ref));
  }
  default:
    return TRAP_ERR_NOSYS;
  }

#undef SYSCALL_ARGS_FROM_USER
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
    if (hal_cpu_is_syscall(frame)) {
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
