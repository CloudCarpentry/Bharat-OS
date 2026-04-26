#include "trap/syscall_context.h"
#include "trap/usercopy.h"
#include "kernel/status.h"
#include "sched/sched.h"
#include "capability.h"
#include "ipc_endpoint.h"
#include "mm.h"
#include "hal/hal.h"
#include "bharat/cpu_local.h"
#include <bharat/uapi/syscall_args.h>

#define TRAP_SUCCESS 0L

// Internal process/cap table helpers
static capability_table_t *get_current_cap_table(bh_syscall_ctx_t *ctx) {
    capability_table_t *table = sched_current_cap_table();
    if (table) return table;
    return NULL;
}

long bh_sys_nop(bh_syscall_ctx_t *ctx) {
    return TRAP_SUCCESS;
}

long bh_sys_thread_create(bh_syscall_ctx_t *ctx) {
    bharat_sys_thread_create_args_t args;
    kstatus_t st = copy_from_user_checked(&args, ctx->regs.arg[0], sizeof(args));
    if (st != K_OK) return kstatus_to_sysret(st);

    bh_process_object_t target;
    st = cap_lookup_process(get_current_cap_table(ctx), args.process_cap, CAP_RIGHT_PROCESS_MANAGE, &target);
    if (st != K_OK) return kstatus_to_sysret(st);

    uint64_t out_tid;
    kstatus_t res = (kstatus_t)sched_sys_thread_create(target.process, (void (*)(void))(uintptr_t)args.entry_point, &out_tid);
    if (res == K_OK) {
        st = copy_to_user_checked(args.out_tid_ptr, &out_tid, sizeof(out_tid));
        if (st != K_OK) return kstatus_to_sysret(st);
    }
    return kstatus_to_sysret(res);
}

long bh_sys_thread_destroy(bh_syscall_ctx_t *ctx) {
    bh_thread_object_t target;
    kstatus_t st = cap_lookup_thread(get_current_cap_table(ctx), (uint32_t)ctx->regs.arg[0], CAP_RIGHT_REVOKE, &target);
    if (st != K_OK) return kstatus_to_sysret(st);

    return kstatus_to_sysret((kstatus_t)sched_sys_thread_destroy(target.tid));
}

long bh_sys_sched_yield(bh_syscall_ctx_t *ctx) {
    bh_thread_yield();
    return TRAP_SUCCESS;
}

long bh_sys_sched_sleep(bh_syscall_ctx_t *ctx) {
    return kstatus_to_sysret((kstatus_t)sched_sys_sleep((uint64_t)ctx->regs.arg[0]));
}

long bh_sys_sched_set_priority(bh_syscall_ctx_t *ctx) {
    bharat_sys_sched_attr_args_t args;
    kstatus_t st = copy_from_user_checked(&args, ctx->regs.arg[0], sizeof(args));
    if (st != K_OK) return kstatus_to_sysret(st);

    bh_thread_object_t target;
    st = cap_lookup_thread(get_current_cap_table(ctx), args.thread_cap, CAP_RIGHT_SCHEDULE, &target);
    if (st != K_OK) return kstatus_to_sysret(st);

    return kstatus_to_sysret((kstatus_t)sched_sys_set_priority(target.tid, (uint32_t)args.value));
}

long bh_sys_sched_set_affinity(bh_syscall_ctx_t *ctx) {
    bharat_sys_sched_attr_args_t args;
    kstatus_t st = copy_from_user_checked(&args, ctx->regs.arg[0], sizeof(args));
    if (st != K_OK) return kstatus_to_sysret(st);

    bh_thread_object_t target;
    st = cap_lookup_thread(get_current_cap_table(ctx), args.thread_cap, CAP_RIGHT_SCHEDULE, &target);
    if (st != K_OK) return kstatus_to_sysret(st);

    return kstatus_to_sysret((kstatus_t)sched_sys_set_affinity(target.tid, (uint32_t)args.value));
}

long bh_sys_vmm_map_page(bh_syscall_ctx_t *ctx) {
    bharat_sys_vmm_map_page_args_t args;
    kstatus_t st = copy_from_user_checked(&args, ctx->regs.arg[0], sizeof(args));
    if (st != K_OK) return kstatus_to_sysret(st);

    // Initial Stage 1 validation of user range
    extern int trap_user_ptr_valid(uintptr_t ptr);
    if (!trap_user_ptr_valid(args.vaddr)) {
        return kstatus_to_sysret(K_ERR_FAULT);
    }

    bh_memory_object_t mem;
    st = cap_lookup_memory(get_current_cap_table(ctx), args.cap_id, CAP_RIGHT_MEMORY_MAP, &mem);
    if (st != K_OK) return kstatus_to_sysret(st);

    return kstatus_to_sysret((kstatus_t)vmm_map_page((virt_addr_t)args.vaddr, (phys_addr_t)mem.base, args.flags));
}

long bh_sys_vmm_unmap_page(bh_syscall_ctx_t *ctx) {
    bharat_sys_vmm_unmap_page_args_t args;
    kstatus_t st = copy_from_user_checked(&args, ctx->regs.arg[0], sizeof(args));
    if (st != K_OK) return kstatus_to_sysret(st);

    extern int trap_user_ptr_valid(uintptr_t ptr);
    if (!trap_user_ptr_valid(args.vaddr)) {
        return kstatus_to_sysret(K_ERR_FAULT);
    }

    bh_memory_object_t mem;
    st = cap_lookup_memory(get_current_cap_table(ctx), args.cap_id, CAP_RIGHT_MEMORY_UNMAP, &mem);
    if (st != K_OK) return kstatus_to_sysret(st);

    return kstatus_to_sysret((kstatus_t)vmm_unmap_page((virt_addr_t)args.vaddr));
}

int cap_invoke(uintptr_t cap_id, uintptr_t opcode, uintptr_t arg0, uintptr_t arg1);

long bh_sys_cap_invoke(bh_syscall_ctx_t *ctx) {
    bharat_sys_cap_invoke_args_t args;
    kstatus_t st = copy_from_user_checked(&args, ctx->regs.arg[0], sizeof(args));
    if (st != K_OK) return kstatus_to_sysret(st);

    return kstatus_to_sysret((kstatus_t)cap_invoke(args.cap_id, args.opcode, args.arg0, args.arg1));
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

long bh_sys_endpoint_create(bh_syscall_ctx_t *ctx) {
    bharat_sys_endpoint_create_args_t args;
    kstatus_t st = copy_from_user_checked(&args, ctx->regs.arg[0], sizeof(args));
    if (st != K_OK) return kstatus_to_sysret(st);

    uint32_t send_cap, recv_cap;
    kstatus_t res = ipc_status_to_kstatus(ipc_endpoint_create(get_current_cap_table(ctx), &send_cap, &recv_cap));
    if (res == K_OK) {
        st = copy_to_user_checked(args.out_send_cap_ptr, &send_cap, sizeof(send_cap));
        if (st != K_OK) return kstatus_to_sysret(st);
        st = copy_to_user_checked(args.out_recv_cap_ptr, &recv_cap, sizeof(recv_cap));
        if (st != K_OK) return kstatus_to_sysret(st);
    }
    return kstatus_to_sysret(res);
}

long bh_sys_endpoint_send(bh_syscall_ctx_t *ctx) {
    bharat_sys_endpoint_send_args_t args;
    kstatus_t st = copy_from_user_checked(&args, ctx->regs.arg[0], sizeof(args));
    if (st != K_OK) return kstatus_to_sysret(st);

    return kstatus_to_sysret(
        ipc_status_to_kstatus(ipc_endpoint_send(get_current_cap_table(ctx), args.send_cap, (const void *)(uintptr_t)args.payload_ptr,
                          args.payload_len, args.timeout_ticks, args.cap_to_send, args.cap_send_rights)));
}

long bh_sys_endpoint_receive(bh_syscall_ctx_t *ctx) {
    bharat_sys_endpoint_receive_args_t args;
    kstatus_t st = copy_from_user_checked(&args, ctx->regs.arg[0], sizeof(args));
    if (st != K_OK) return kstatus_to_sysret(st);

    uint32_t len_received, cap_received;
    kstatus_t res = ipc_status_to_kstatus(ipc_endpoint_receive(get_current_cap_table(ctx), args.recv_cap, (void *)(uintptr_t)args.out_payload_ptr,
                             args.out_payload_capacity, &len_received, args.timeout_ticks, &cap_received));

    if (res == K_OK) {
        st = copy_to_user_checked(args.out_len_ptr, &len_received, sizeof(len_received));
        if (st != K_OK) return kstatus_to_sysret(st);
        if (args.out_received_cap_ptr) {
            st = copy_to_user_checked(args.out_received_cap_ptr, &cap_received, sizeof(cap_received));
            if (st != K_OK) return kstatus_to_sysret(st);
        }
    }
    return kstatus_to_sysret(res);
}

long bh_sys_cap_delegate(bh_syscall_ctx_t *ctx) {
    bharat_sys_cap_delegate_args_t args;
    kstatus_t st = copy_from_user_checked(&args, ctx->regs.arg[0], sizeof(args));
    if (st != K_OK) return kstatus_to_sysret(st);

    uint32_t out_cap;
    kstatus_t res = (kstatus_t)cap_table_delegate(get_current_cap_table(ctx), get_current_cap_table(ctx), args.src_cap, args.requested_rights, &out_cap);
    if (res == K_OK) {
        st = copy_to_user_checked(args.out_cap_ptr, &out_cap, sizeof(out_cap));
        if (st != K_OK) return kstatus_to_sysret(st);
    }
    return kstatus_to_sysret(res);
}

int sched_sys_intent_set(uint64_t tid, const void* intent);
int sched_sys_intent_get(uint64_t tid, void* intent);
#include <bharat/uapi/system/intent.h>

long bh_sys_intent_set(bh_syscall_ctx_t *ctx) {
    bharat_sys_intent_args_t args;
    kstatus_t st = copy_from_user_checked(&args, ctx->regs.arg[0], sizeof(args));
    if (st != K_OK) return kstatus_to_sysret(st);

    bh_thread_object_t target;
    st = cap_lookup_thread(get_current_cap_table(ctx), args.thread_cap, CAP_RIGHT_SCHEDULE, &target);
    if (st != K_OK) return kstatus_to_sysret(st);

    bharat_intent_t intent;
    st = copy_from_user_checked(&intent, args.intent_ptr, sizeof(intent));
    if (st != K_OK) return kstatus_to_sysret(st);

    return kstatus_to_sysret((kstatus_t)sched_sys_intent_set(target.tid, &intent));
}

long bh_sys_intent_get(bh_syscall_ctx_t *ctx) {
    bharat_sys_intent_args_t args;
    kstatus_t st = copy_from_user_checked(&args, ctx->regs.arg[0], sizeof(args));
    if (st != K_OK) return kstatus_to_sysret(st);

    bh_thread_object_t target;
    st = cap_lookup_thread(get_current_cap_table(ctx), args.thread_cap, CAP_RIGHT_SCHEDULE, &target);
    if (st != K_OK) return kstatus_to_sysret(st);

    bharat_intent_t intent;
    kstatus_t res = (kstatus_t)sched_sys_intent_get(target.tid, &intent);
    if (res == K_OK) {
        st = copy_to_user_checked(args.intent_ptr, &intent, sizeof(intent));
        if (st != K_OK) return kstatus_to_sysret(st);
    }
    return kstatus_to_sysret(res);
}

int sys_mem_alloc_class(size_t size, uint32_t mem_class, uint32_t flags, uint64_t* out_addr);

long bh_sys_mem_alloc_class(bh_syscall_ctx_t *ctx) {
    bharat_sys_mem_alloc_args_t args;
    kstatus_t st = copy_from_user_checked(&args, ctx->regs.arg[0], sizeof(args));
    if (st != K_OK) return kstatus_to_sysret(st);

    bh_process_object_t target;
    st = cap_lookup_process(get_current_cap_table(ctx), args.resource_cap, CAP_RIGHT_RESOURCE_ALLOC, &target);
    if (st != K_OK) return kstatus_to_sysret(st);

    uint64_t out_addr;
    kstatus_t res = (kstatus_t)sys_mem_alloc_class((size_t)args.size, (uint32_t)args.mem_class, (uint32_t)args.flags, &out_addr);
    if (res == K_OK) {
        st = copy_to_user_checked(args.out_addr_ptr, &out_addr, sizeof(out_addr));
        if (st != K_OK) return kstatus_to_sysret(st);
    }
    return kstatus_to_sysret(res);
}

int sys_fault_domain_create(const void* attr, uint64_t* out_domain);
int sys_fault_domain_destroy(uint64_t domain);
int sys_fault_domain_attach(uint64_t domain, uint64_t tid);
#include <bharat/uapi/system/fault_domain.h>

long bh_sys_fault_domain_create(bh_syscall_ctx_t *ctx) {
    bharat_sys_fault_domain_args_t args;
    kstatus_t st = copy_from_user_checked(&args, ctx->regs.arg[0], sizeof(args));
    if (st != K_OK) return kstatus_to_sysret(st);

    bh_process_object_t target;
    st = cap_lookup_process(get_current_cap_table(ctx), args.cap_id, CAP_RIGHT_FAULT_DOMAIN_MANAGE, &target);
    if (st != K_OK) return kstatus_to_sysret(st);

    bharat_fault_domain_attr_t attr;
    st = copy_from_user_checked(&attr, args.attr_ptr, sizeof(attr));
    if (st != K_OK) return kstatus_to_sysret(st);

    uint64_t out_domain;
    kstatus_t res = (kstatus_t)sys_fault_domain_create(&attr, &out_domain);
    if (res == K_OK) {
        st = copy_to_user_checked(args.out_domain_ptr, &out_domain, sizeof(out_domain));
        if (st != K_OK) return kstatus_to_sysret(st);
    }
    return kstatus_to_sysret(res);
}

long bh_sys_fault_domain_destroy(bh_syscall_ctx_t *ctx) {
    bh_process_object_t target;
    kstatus_t st = cap_lookup_process(get_current_cap_table(ctx), (uint32_t)ctx->regs.arg[0], CAP_RIGHT_FAULT_DOMAIN_MANAGE, &target);
    if (st != K_OK) return kstatus_to_sysret(st);

    return kstatus_to_sysret((kstatus_t)sys_fault_domain_destroy((uint64_t)target.process)); // Still using object_ref for now
}

long bh_sys_fault_domain_attach(bh_syscall_ctx_t *ctx) {
    bharat_sys_fault_domain_args_t args;
    kstatus_t st = copy_from_user_checked(&args, ctx->regs.arg[0], sizeof(args));
    if (st != K_OK) return kstatus_to_sysret(st);

    bh_process_object_t domain_proc;
    st = cap_lookup_process(get_current_cap_table(ctx), args.cap_id, CAP_RIGHT_FAULT_DOMAIN_MANAGE, &domain_proc);
    if (st != K_OK) return kstatus_to_sysret(st);

    bh_thread_object_t target_thread;
    st = cap_lookup_thread(get_current_cap_table(ctx), args.thread_cap, CAP_RIGHT_SCHEDULE, &target_thread);
    if (st != K_OK) return kstatus_to_sysret(st);

    return kstatus_to_sysret((kstatus_t)sys_fault_domain_attach((uint64_t)domain_proc.process, target_thread.tid));
}

long bh_sys_read(bh_syscall_ctx_t *ctx) {
    (void)ctx;
    return TRAP_SUCCESS;
}

long bh_sys_thread_exit(bh_syscall_ctx_t *ctx) {
    bh_thread_t *current = sched_current_thread();
    if (current) {
        // status is in ctx->regs.arg[0]
        (void)sched_mark_thread_terminated(current);

        // Since we are terminating the current thread, we must never return to user space.
        // We clear the current thread and trigger an immediate reschedule.
        uint32_t core = hal_cpu_get_id();
        // Accessing g_cpu_locals requires including its header or extern
        extern cpu_local_t g_cpu_locals[];
        g_cpu_locals[core].runqueue.current_thread = NULL;
        bh_thread_yield();

        // Should never reach here
        while(1);
    }
    return kstatus_to_sysret(K_ERR_BAD_THREAD);
}

long bh_sys_write(bh_syscall_ctx_t *ctx) {
    (void)ctx;
    return TRAP_SUCCESS;
}

long bh_sys_get_subsystem_caps(bh_syscall_ctx_t *ctx) {
    (void)ctx;
    return TRAP_SUCCESS;
}
