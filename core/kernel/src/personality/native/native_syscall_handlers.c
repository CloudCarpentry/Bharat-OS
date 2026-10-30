#include "trap/syscall_context.h"
#include "syscall/usercopy.h"
#include "trap/syscall_status.h"
#include "kernel/status.h"
#include "sched/sched.h"
#include "capability.h"
#include "capability/cap_lookup.h"
#include "syscall/syscall_capability.h"
#include "ipc_endpoint.h"
#include "mm.h"
#include "hal/hal.h"
#include "bharat/cpu_local.h"
#include <bharat/uapi/syscall_args.h>
#include <bharat/uapi/capability/rights.h>
#include <bharat/uapi/time/time.h>
#include "time/ktime.h"

#define TRAP_SUCCESS 0L

bh_operation_result_t bh_sys_nop(bh_syscall_ctx_t *ctx) {
    return bh_op_result_kstatus(K_OK);
}

bh_operation_result_t bh_sys_time_get(bh_syscall_ctx_t *ctx) {
    uint64_t now_ns;
    bh_status_t st;

    if ((uint32_t)ctx->regs.arg[0] != BH_CLOCK_MONOTONIC) {
        return bh_op_result_kstatus(K_ERR_UNSUPPORTED);
    }
    now_ns = bh_ktime_now();
    st = bh_copy_to_user((void *)ctx->regs.arg[1], &now_ns, sizeof(now_ns));
    return bh_op_result_kstatus(bh_status_to_kstatus(st));
}

bh_operation_result_t bh_sys_thread_create(bh_syscall_ctx_t *ctx) {
    bharat_sys_thread_create_args_t args;
    bh_status_t st = bh_copy_from_user(&args, (const void *)ctx->regs.arg[0], sizeof(args));
    if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

    bh_process_object_t target;
    kstatus_t kst = bh_syscall_cap_lookup_process(ctx, args.process_cap, CAP_RIGHT_RESOURCE_ALLOC, &target);
    if (kst != K_OK) return bh_op_result_kstatus(kst);

    uint64_t out_tid;
    kstatus_t res = (kstatus_t)sched_sys_thread_create(target.process, (void (*)(void))(uintptr_t)args.entry_point, &out_tid);
    if (res == K_OK) {
        st = bh_copy_to_user((void *)args.out_tid_ptr, &out_tid, sizeof(out_tid));
        if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));
    }
    return bh_op_result_kstatus(res);
}

bh_operation_result_t bh_sys_thread_destroy(bh_syscall_ctx_t *ctx) {
    bh_thread_object_t target;
    kstatus_t kst = bh_syscall_cap_lookup_thread(ctx, (uint32_t)ctx->regs.arg[0], CAP_RIGHT_PROCESS_MANAGE, &target);
    if (kst != K_OK) return bh_op_result_kstatus(kst);

    return bh_op_result_kstatus((kstatus_t)sched_sys_thread_destroy(target.tid));
}

bh_operation_result_t bh_sys_sched_yield(bh_syscall_ctx_t *ctx) {
    bh_thread_yield();
    return bh_op_result_kstatus(K_OK);
}

bh_operation_result_t bh_sys_sched_sleep(bh_syscall_ctx_t *ctx) {
    return bh_op_result_kstatus((kstatus_t)sched_sys_sleep((uint64_t)ctx->regs.arg[0]));
}

bh_operation_result_t bh_sys_sched_set_priority(bh_syscall_ctx_t *ctx) {
    bharat_sys_sched_attr_args_t args;
    bh_status_t st = bh_copy_from_user(&args, (const void *)ctx->regs.arg[0], sizeof(args));
    if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

    bh_thread_object_t target;
    kstatus_t kst = bh_syscall_cap_lookup_thread(ctx, args.thread_cap, CAP_RIGHT_SCHEDULE, &target);
    if (kst != K_OK) return bh_op_result_kstatus(kst);

    return bh_op_result_kstatus((kstatus_t)sched_sys_set_priority(target.tid, (uint32_t)args.value));
}

bh_operation_result_t bh_sys_sched_set_affinity(bh_syscall_ctx_t *ctx) {
    bharat_sys_sched_attr_args_t args;
    bh_status_t st = bh_copy_from_user(&args, (const void *)ctx->regs.arg[0], sizeof(args));
    if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

    bh_thread_object_t target;
    kstatus_t kst = bh_syscall_cap_lookup_thread(ctx, args.thread_cap, CAP_RIGHT_SCHEDULE, &target);
    if (kst != K_OK) return bh_op_result_kstatus(kst);

    return bh_op_result_kstatus((kstatus_t)sched_sys_set_affinity(target.tid, (uint32_t)args.value));
}

bh_operation_result_t bh_sys_vmm_map_page(bh_syscall_ctx_t *ctx) {
    bharat_sys_vmm_map_page_args_t args;
    bh_status_t st = bh_copy_from_user(&args, (const void *)ctx->regs.arg[0], sizeof(args));
    if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

    st = bh_user_range_validate((const void *)args.vaddr, PAGE_SIZE, BH_USER_ACCESS_READ);
    if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

    bh_memory_object_t mem;
    kstatus_t kst = bh_syscall_cap_lookup_memory(ctx, args.cap_id, CAP_RIGHT_MEMORY_MAP, &mem);
    if (kst != K_OK) return bh_op_result_kstatus(kst);

    return bh_op_result_kstatus((kstatus_t)vmm_map_page((virt_addr_t)args.vaddr, (phys_addr_t)mem.base, args.flags));
}

bh_operation_result_t bh_sys_vmm_unmap_page(bh_syscall_ctx_t *ctx) {
    bharat_sys_vmm_unmap_page_args_t args;
    bh_status_t st = bh_copy_from_user(&args, (const void *)ctx->regs.arg[0], sizeof(args));
    if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

    st = bh_user_range_validate((const void *)args.vaddr, PAGE_SIZE, BH_USER_ACCESS_READ);
    if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

    bh_memory_object_t mem;
    kstatus_t kst = bh_syscall_cap_lookup_memory(ctx, args.cap_id, CAP_RIGHT_MEMORY_UNMAP, &mem);
    if (kst != K_OK) return bh_op_result_kstatus(kst);

    return bh_op_result_kstatus((kstatus_t)vmm_unmap_page((virt_addr_t)args.vaddr));
}

int cap_invoke(uintptr_t cap_id, uintptr_t opcode, uintptr_t arg0, uintptr_t arg1);

bh_operation_result_t bh_sys_cap_invoke(bh_syscall_ctx_t *ctx) {
    bharat_sys_cap_invoke_args_t args;
    bh_status_t st = bh_copy_from_user(&args, (const void *)ctx->regs.arg[0], sizeof(args));
    if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

    return bh_op_result_kstatus((kstatus_t)cap_invoke(args.cap_id, args.opcode, args.arg0, args.arg1));
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
        case IPC_ERR_CANCELLED: return K_ERR_CANCELLED;
        case IPC_ERR_REVOKED: return K_ERR_CAP_REVOKED;
        default: return K_ERR_INTERNAL_BUG;
    }
}

bh_operation_result_t bh_sys_endpoint_create(bh_syscall_ctx_t *ctx) {
    bharat_sys_endpoint_create_args_t args;
    bh_status_t st = bh_copy_from_user(&args, (const void *)ctx->regs.arg[0], sizeof(args));
    if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

    capability_table_t *table = (capability_table_t *)ctx->process->security_sandbox_ctx;
    if (!table) return bh_op_result_kstatus(K_ERR_DENIED);

    uint32_t send_cap, recv_cap;
    kstatus_t res = ipc_status_to_kstatus(ipc_endpoint_create(table, &send_cap, &recv_cap));
    if (res == K_OK) {
        st = bh_copy_to_user((void *)args.out_send_cap_ptr, &send_cap, sizeof(send_cap));
        if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));
        st = bh_copy_to_user((void *)args.out_recv_cap_ptr, &recv_cap, sizeof(recv_cap));
        if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));
    }
    return bh_op_result_kstatus(res);
}

bh_operation_result_t bh_sys_endpoint_send(bh_syscall_ctx_t *ctx) {
    bharat_sys_endpoint_send_args_t args;
    bh_status_t st = bh_copy_from_user(&args, (const void *)ctx->regs.arg[0], sizeof(args));
    if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

    bh_endpoint_object_t ep;
    kstatus_t kst = bh_syscall_cap_lookup_endpoint(ctx, args.send_cap, CAP_RIGHT_ENDPOINT_SEND, &ep);
    if (kst != K_OK) return bh_op_result_kstatus(kst);

    uint8_t payload[BHARAT_IPC_ENDPOINT_PAYLOAD_MAX];
    if (!args.payload_len || args.payload_len > sizeof(payload))
        return bh_op_result_kstatus(K_ERR_INVALID_ARG);
    st = bh_copy_from_user(payload, (const void *)(uintptr_t)args.payload_ptr, args.payload_len);
    if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

    if (!ctx->process || !ctx->process->security_sandbox_ctx)
        return bh_op_result_kstatus(K_ERR_DENIED);
    capability_table_t *table = (capability_table_t *)ctx->process->security_sandbox_ctx;

    ipc_provenance_t prov = {0};
    prov.sender_pid = ctx->process->process_id;
    prov.sender_process_object_id = ctx->process->object_id;
    prov.sender_cspace_id = table->cspace_id;
    prov.sender_owner_core = table->owner_core;
    prov.has_provenance = 1U;
    prov.reply_cap_to_send = args.reply_cap_to_send;
    prov.reply_cap_send_rights = args.reply_cap_send_rights;

    return bh_op_result_kstatus(
        ipc_status_to_kstatus(ipc_endpoint_send_ex(table, args.send_cap, payload,
                          args.payload_len, args.timeout_ticks, args.cap_to_send, args.cap_send_rights, &prov)));
}

bh_operation_result_t bh_sys_endpoint_receive(bh_syscall_ctx_t *ctx) {
    uint32_t selector = (uint32_t)ctx->regs.arg[1];
    if (selector == BH_IPC_RECEIVE_SELECTOR_LEGACY) {
        bharat_sys_endpoint_receive_args_t args;
        bh_status_t st = bh_copy_from_user(&args, (const void *)ctx->regs.arg[0], sizeof(args));
        if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

        bh_endpoint_object_t ep;
        kstatus_t kst = bh_syscall_cap_lookup_endpoint(ctx, args.recv_cap, CAP_RIGHT_ENDPOINT_RECEIVE, &ep);
        if (kst != K_OK) return bh_op_result_kstatus(kst);

        st = bh_user_range_validate((void *)(uintptr_t)args.out_payload_ptr, args.out_payload_capacity, BH_USER_ACCESS_WRITE);
        if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));
        st = bh_user_range_validate((void *)(uintptr_t)args.out_len_ptr, sizeof(uint32_t), BH_USER_ACCESS_WRITE);
        if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));
        if (args.out_received_cap_ptr) {
            st = bh_user_range_validate((void *)(uintptr_t)args.out_received_cap_ptr, sizeof(uint32_t), BH_USER_ACCESS_WRITE);
            if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));
        }

        if (!ctx->process || !ctx->process->security_sandbox_ctx)
            return bh_op_result_kstatus(K_ERR_DENIED);
        capability_table_t *table = (capability_table_t *)ctx->process->security_sandbox_ctx;

        uint8_t payload[BHARAT_IPC_ENDPOINT_PAYLOAD_MAX];
        uint32_t capacity = args.out_payload_capacity < sizeof(payload) ? args.out_payload_capacity : sizeof(payload);
        uint32_t len_received = 0, cap_received = 0;
        kstatus_t res = ipc_status_to_kstatus(ipc_endpoint_receive(table, args.recv_cap, payload,
                                 capacity, &len_received, args.timeout_ticks, &cap_received));

        if (res == K_OK) {
            st = bh_copy_to_user((void *)(uintptr_t)args.out_payload_ptr, payload, len_received);
            if (st != BH_OK) goto legacy_copy_failed;
            st = bh_copy_to_user((void *)(uintptr_t)args.out_len_ptr, &len_received, sizeof(len_received));
            if (st != BH_OK) goto legacy_copy_failed;
            if (args.out_received_cap_ptr) {
                st = bh_copy_to_user((void *)(uintptr_t)args.out_received_cap_ptr, &cap_received, sizeof(cap_received));
                if (st != BH_OK) goto legacy_copy_failed;
            } else if (cap_received && cap_table_revoke(table, cap_received) != 0) {
                return bh_op_result_kstatus(K_ERR_BAD_STATE);
            }
        }
        return bh_op_result_kstatus(res);
legacy_copy_failed:
        /* A transferred handle is never retained silently after failed copyout.
         * Failed revocation leaves the object quarantined in the calling CSpace. */
        if (cap_received && cap_table_revoke(table, cap_received) != 0)
            return bh_op_result_kstatus(K_ERR_BAD_STATE);
        return bh_op_result_kstatus(bh_status_to_kstatus(st));
    } else if (selector == BH_IPC_RECEIVE_SELECTOR_V1) {
        bharat_sys_endpoint_receive_v1_args_t args;
        bh_status_t st = bh_copy_from_user(&args, (const void *)ctx->regs.arg[0], sizeof(args));
        if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

        if (args.version != BH_IPC_RECEIVE_ENVELOPE_VERSION_V1) {
            return bh_op_result_kstatus(K_ERR_INVALID_ARG);
        }
        if (args.struct_size != sizeof(bharat_sys_endpoint_receive_v1_args_t)) {
            return bh_op_result_kstatus(K_ERR_INVALID_ARG);
        }
        if (args.flags != 0U || args.reserved0 != 0U) {
            return bh_op_result_kstatus(K_ERR_INVALID_ARG);
        }
        if (args.out_payload_capacity == 0U || args.out_payload_ptr == 0U ||
            args.out_len_ptr == 0U || args.out_envelope_ptr == 0U) {
            return bh_op_result_kstatus(K_ERR_INVALID_ARG);
        }
        if (args.out_envelope_capacity < sizeof(bh_ipc_receive_envelope_v1_t)) {
            return bh_op_result_kstatus(K_ERR_INVALID_ARG);
        }

#if UINTPTR_MAX == UINT32_MAX
        if ((args.out_payload_ptr >> 32) != 0 ||
            (args.out_len_ptr >> 32) != 0 ||
            (args.out_envelope_ptr >> 32) != 0) {
            return bh_op_result_kstatus(K_ERR_INVALID_ARG);
        }
#endif

        bh_endpoint_object_t ep;
        kstatus_t kst = bh_syscall_cap_lookup_endpoint(ctx, args.recv_cap, CAP_RIGHT_ENDPOINT_RECEIVE, &ep);
        if (kst != K_OK) return bh_op_result_kstatus(kst);

        st = bh_user_range_validate((void *)(uintptr_t)args.out_payload_ptr, args.out_payload_capacity, BH_USER_ACCESS_WRITE);
        if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));
        st = bh_user_range_validate((void *)(uintptr_t)args.out_len_ptr, sizeof(uint32_t), BH_USER_ACCESS_WRITE);
        if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));
        st = bh_user_range_validate((void *)(uintptr_t)args.out_envelope_ptr, sizeof(bh_ipc_receive_envelope_v1_t), BH_USER_ACCESS_WRITE);
        if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

        if (!ctx->process || !ctx->process->security_sandbox_ctx)
            return bh_op_result_kstatus(K_ERR_DENIED);
        capability_table_t *table = (capability_table_t *)ctx->process->security_sandbox_ctx;

        uint8_t payload[BHARAT_IPC_ENDPOINT_PAYLOAD_MAX];
        uint32_t capacity = args.out_payload_capacity < sizeof(payload) ? args.out_payload_capacity : sizeof(payload);
        uint32_t len_received = 0;
        bh_ipc_receive_envelope_v1_t envelope = {0};

        kstatus_t res = ipc_status_to_kstatus(ipc_endpoint_receive_envelope(table, args.recv_cap, payload,
                                 capacity, &len_received, args.timeout_ticks, &envelope));

        if (res == K_OK) {
            st = bh_copy_to_user((void *)(uintptr_t)args.out_payload_ptr, payload, len_received);
            if (st != BH_OK) goto v1_copy_failed;
            st = bh_copy_to_user((void *)(uintptr_t)args.out_len_ptr, &len_received, sizeof(len_received));
            if (st != BH_OK) goto v1_copy_failed;
            st = bh_copy_to_user((void *)(uintptr_t)args.out_envelope_ptr, &envelope, sizeof(envelope));
            if (st != BH_OK) goto v1_copy_failed;
        }
        return bh_op_result_kstatus(res);

v1_copy_failed:
        /* Rollback any capability installed into the caller's CSpace */
        if (envelope.reply_send_cap != 0U) {
            if (cap_table_revoke(table, envelope.reply_send_cap) != 0) {
                return bh_op_result_kstatus(K_ERR_BAD_STATE);
            }
        }
        if (envelope.authority_cap != 0U) {
            if (cap_table_revoke(table, envelope.authority_cap) != 0) {
                return bh_op_result_kstatus(K_ERR_BAD_STATE);
            }
        }
        return bh_op_result_kstatus(bh_status_to_kstatus(st));
    } else {
        return bh_op_result_kstatus(K_ERR_INVALID_ARG);
    }
}

bh_operation_result_t bh_sys_cap_delegate(bh_syscall_ctx_t *ctx) {
    bharat_sys_cap_delegate_args_t args;
    bh_status_t st = bh_copy_from_user(&args, (const void *)ctx->regs.arg[0], sizeof(args));
    if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

    bh_status_t bh_st = bh_syscall_validate_capability(ctx, args.src_cap, CAP_TYPE_NONE, CAP_RIGHT_DELEGATE);
    if (bh_st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(bh_st));

    capability_table_t *table = (capability_table_t *)ctx->process->security_sandbox_ctx;
    if (!table) return bh_op_result_kstatus(K_ERR_DENIED);

    uint32_t out_cap;
    kstatus_t res = (kstatus_t)cap_table_delegate(table, table, args.src_cap, args.requested_rights, &out_cap);
    if (res == K_OK) {
        st = bh_copy_to_user((void *)args.out_cap_ptr, &out_cap, sizeof(out_cap));
        if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));
    }
    return bh_op_result_kstatus(res);
}

#include <bharat/uapi/system/intent.h>

bh_operation_result_t bh_sys_intent_set(bh_syscall_ctx_t *ctx) {
    bharat_sys_intent_args_t args;
    bh_status_t st = bh_copy_from_user(&args, (const void *)ctx->regs.arg[0], sizeof(args));
    if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

    bh_thread_object_t target;
    kstatus_t kst = bh_syscall_cap_lookup_thread(ctx, args.thread_cap, CAP_RIGHT_PROCESS_MANAGE, &target);
    if (kst != K_OK) return bh_op_result_kstatus(kst);

    bharat_intent_t intent;
    st = bh_copy_from_user(&intent, (const void *)args.intent_ptr, sizeof(intent));
    if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

    return bh_op_result_kstatus((kstatus_t)sched_sys_intent_set(target.tid, &intent));
}

bh_operation_result_t bh_sys_intent_get(bh_syscall_ctx_t *ctx) {
    bharat_sys_intent_args_t args;
    bh_status_t st = bh_copy_from_user(&args, (const void *)ctx->regs.arg[0], sizeof(args));
    if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

    bh_thread_object_t target;
    kstatus_t kst = bh_syscall_cap_lookup_thread(ctx, args.thread_cap, CAP_RIGHT_PROCESS_MANAGE, &target);
    if (kst != K_OK) return bh_op_result_kstatus(kst);

    bharat_intent_t intent;
    kstatus_t res = (kstatus_t)sched_sys_intent_get(target.tid, &intent);
    if (res == K_OK) {
        st = bh_copy_to_user((void *)args.intent_ptr, &intent, sizeof(intent));
        if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));
    }
    return bh_op_result_kstatus(res);
}

int sys_mem_alloc_class(size_t size, uint32_t mem_class, uint32_t flags, uint64_t* out_addr);

bh_operation_result_t bh_sys_mem_alloc_class(bh_syscall_ctx_t *ctx) {
    bharat_sys_mem_alloc_args_t args;
    bh_status_t st = bh_copy_from_user(&args, (const void *)ctx->regs.arg[0], sizeof(args));
    if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

    bh_process_object_t target;
    kstatus_t kst = bh_syscall_cap_lookup_process(ctx, args.resource_cap, CAP_RIGHT_RESOURCE_ALLOC, &target);
    if (kst != K_OK) return bh_op_result_kstatus(kst);

    uint64_t out_addr;
    kstatus_t res = (kstatus_t)sys_mem_alloc_class((size_t)args.size, (uint32_t)args.mem_class, (uint32_t)args.flags, &out_addr);
    if (res == K_OK) {
        st = bh_copy_to_user((void *)args.out_addr_ptr, &out_addr, sizeof(out_addr));
        if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));
    }
    return bh_op_result_kstatus(res);
}

int sys_fault_domain_create(const void* attr, uint64_t* out_domain);
int sys_fault_domain_destroy(uint64_t domain);
int sys_fault_domain_attach(uint64_t domain, uint64_t tid);
#include <bharat/uapi/system/fault_domain.h>

bh_operation_result_t bh_sys_fault_domain_create(bh_syscall_ctx_t *ctx) {
    bharat_sys_fault_domain_args_t args;
    bh_status_t st = bh_copy_from_user(&args, (const void *)ctx->regs.arg[0], sizeof(args));
    if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

    bh_process_object_t target;
    kstatus_t kst = bh_syscall_cap_lookup_process(ctx, args.cap_id, CAP_RIGHT_RESOURCE_ALLOC, &target);
    if (kst != K_OK) return bh_op_result_kstatus(kst);

    bharat_fault_domain_attr_t attr;
    st = bh_copy_from_user(&attr, (const void *)args.attr_ptr, sizeof(attr));
    if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

    uint64_t out_domain;
    kstatus_t res = (kstatus_t)sys_fault_domain_create(&attr, &out_domain);
    if (res == K_OK) {
        st = bh_copy_to_user((void *)args.out_domain_ptr, &out_domain, sizeof(out_domain));
        if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));
    }
    return bh_op_result_kstatus(res);
}

bh_operation_result_t bh_sys_fault_domain_destroy(bh_syscall_ctx_t *ctx) {
    void *domain_ref;
    kstatus_t kst = bh_syscall_cap_lookup_fault_domain(ctx, (uint32_t)ctx->regs.arg[0], CAP_RIGHT_FAULT_DOMAIN_MANAGE, &domain_ref);
    if (kst != K_OK) return bh_op_result_kstatus(kst);

    return bh_op_result_kstatus((kstatus_t)sys_fault_domain_destroy((uintptr_t)domain_ref));
}

bh_operation_result_t bh_sys_fault_domain_attach(bh_syscall_ctx_t *ctx) {
    bharat_sys_fault_domain_args_t args;
    bh_status_t st = bh_copy_from_user(&args, (const void *)ctx->regs.arg[0], sizeof(args));
    if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

    void *domain_ref;
    kstatus_t kst = bh_syscall_cap_lookup_fault_domain(ctx, args.cap_id, CAP_RIGHT_FAULT_DOMAIN_MANAGE, &domain_ref);
    if (kst != K_OK) return bh_op_result_kstatus(kst);

    bh_thread_object_t target_thread;
    kst = bh_syscall_cap_lookup_thread(ctx, args.thread_cap, CAP_RIGHT_PROCESS_MANAGE, &target_thread);
    if (kst != K_OK) return bh_op_result_kstatus(kst);

    return bh_op_result_kstatus((kstatus_t)sys_fault_domain_attach((uintptr_t)domain_ref, target_thread.tid));
}

bh_operation_result_t bh_sys_read(bh_syscall_ctx_t *ctx) {
    return bh_op_result_kstatus(K_ERR_UNSUPPORTED);
}

bh_operation_result_t bh_sys_thread_exit(bh_syscall_ctx_t *ctx) {
    bh_thread_t *current = sched_current_thread();
    if (current) {
        (void)sched_mark_thread_terminated(current);
        uint32_t core = hal_cpu_get_id();
        extern cpu_local_t g_cpu_locals[];
        g_cpu_locals[core].runqueue.current_thread = NULL;
        bh_thread_yield();
        while(1);
    }
    return bh_op_result_kstatus(K_ERR_BAD_STATE);
}

#include "console/console_core.h"
#define BH_CONSOLE_WRITE_MAX_BYTES 4096

bh_operation_result_t bh_sys_write(bh_syscall_ctx_t *ctx) {
    int fd = (int)ctx->regs.arg[0];
    uintptr_t user_buf = ctx->regs.arg[1];
    size_t count = (size_t)ctx->regs.arg[2];

    if (fd != 1 && fd != 2)
        return bh_op_result_kstatus(K_ERR_UNSUPPORTED);

    if (count > BH_CONSOLE_WRITE_MAX_BYTES)
        return bh_op_result_kstatus(K_ERR_INVALID_ARG);

    size_t done = 0;

    while (done < count) {
        char temp[CONSOLE_MAX_RECORD_TEXT - 1];

        size_t n = count - done;
        if (n > sizeof(temp))
            n = sizeof(temp);

        bh_status_t st =
            bh_copy_from_user(temp,
                              (const void *)(user_buf + done),
                              n);

        if (st != BH_OK)
            return done ? bh_op_result_value((long)done) : bh_op_result_kstatus(bh_status_to_kstatus(st));

        console_write_raw(temp, n);
        done += n;
    }

    return bh_op_result_value((long)done);
}

bh_operation_result_t bh_sys_get_subsystem_caps(bh_syscall_ctx_t *ctx) {
    return bh_op_result_kstatus(K_ERR_UNSUPPORTED);
}

#include <bharat/memory/hmem.h>
#include <mm/hmem.h>
#include <mm/prot_domain.h>
#include <mm/tlb.h>
#include <mm/aspace.h>
#include <mm/vm_mapping.h>
#include "lib/base/string.h"

static inline cap_rights_mask_t hmem_rights_to_cap_rights(uint64_t hmem_rights) {
    cap_rights_mask_t r = 0;
    if (hmem_rights & BH_HMEM_RIGHT_READ) r |= (CAP_RIGHT_READ | BH_CAP_RIGHT_READ);
    if (hmem_rights & BH_HMEM_RIGHT_WRITE) r |= (CAP_RIGHT_WRITE | BH_CAP_RIGHT_WRITE);
    if (hmem_rights & BH_HMEM_RIGHT_MAP_CPU) r |= (CAP_RIGHT_HMEM_MAP_CPU | CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_MEMORY_UNMAP | BH_CAP_RIGHT_MAP | BH_CAP_RIGHT_UNMAP);
    if (hmem_rights & BH_HMEM_RIGHT_MAP_DEVICE) r |= (CAP_RIGHT_HMEM_MAP_DEVICE | CAP_RIGHT_DEVICE_DMA);
    if (hmem_rights & BH_HMEM_RIGHT_SHARE) r |= (CAP_RIGHT_HMEM_SHARE | CAP_RIGHT_MEMORY_SHARE);
    if (hmem_rights & BH_HMEM_RIGHT_QUERY) r |= (CAP_RIGHT_QUERY | CAP_RIGHT_HMEM_QUERY | BH_CAP_RIGHT_QUERY);
    if (hmem_rights & BH_HMEM_RIGHT_DESTROY) r |= (CAP_RIGHT_HMEM_DESTROY | BH_CAP_RIGHT_DESTROY);
    return r;
}

static inline kstatus_t bh_syscall_cap_lookup_hmem(bh_syscall_ctx_t *ctx,
                                                   uintptr_t cap_id,
                                                   uint64_t required_rights,
                                                   bh_hmem_t **out_hmem) {
    if (!ctx || !ctx->process || !ctx->process->security_sandbox_ctx) {
        return K_ERR_DENIED;
    }
    if (!bh_cap_is_valid_encoding((uint32_t)cap_id)) {
        return K_ERR_CAP_INVALID;
    }
    cap_validation_request_t req = {
        .cap_id = (uint32_t)cap_id,
        .expected_object_type = CAP_TYPE_HMEM,
        .required_rights = required_rights,
        .requester_pid = ctx->process->process_id,
        .expected_generation = bh_cap_generation((uint32_t)cap_id)
    };
    capability_entry_t entry;
    kstatus_t st = cap_validate_ex((capability_table_t *)ctx->process->security_sandbox_ctx,
                                   &req, &entry);
    if (st == K_OK) {
        if (out_hmem) {
            *out_hmem = (bh_hmem_t *)(uintptr_t)entry.object_ref;
        }
    }
    return st;
}

#if defined(BHARAT_PROFILE_MPU_ONLY) || defined(PROFILE_MPU_ONLY)

bh_operation_result_t bh_sys_hmem_create(bh_syscall_ctx_t *ctx) {
    (void)ctx;
    return bh_op_result_kstatus(K_ERR_UNSUPPORTED);
}
bh_operation_result_t bh_sys_hmem_destroy(bh_syscall_ctx_t *ctx) {
    (void)ctx;
    return bh_op_result_kstatus(K_ERR_UNSUPPORTED);
}
bh_operation_result_t bh_sys_hmem_query(bh_syscall_ctx_t *ctx) {
    (void)ctx;
    return bh_op_result_kstatus(K_ERR_UNSUPPORTED);
}
bh_operation_result_t bh_sys_hmem_map_cpu(bh_syscall_ctx_t *ctx) {
    (void)ctx;
    return bh_op_result_kstatus(K_ERR_UNSUPPORTED);
}
bh_operation_result_t bh_sys_hmem_unmap_cpu(bh_syscall_ctx_t *ctx) {
    (void)ctx;
    return bh_op_result_kstatus(K_ERR_UNSUPPORTED);
}
bh_operation_result_t bh_sys_hmem_sync_for_cpu(bh_syscall_ctx_t *ctx) {
    (void)ctx;
    return bh_op_result_kstatus(K_ERR_UNSUPPORTED);
}
bh_operation_result_t bh_sys_hmem_sync_for_device(bh_syscall_ctx_t *ctx) {
    (void)ctx;
    return bh_op_result_kstatus(K_ERR_UNSUPPORTED);
}

#else

bh_operation_result_t bh_sys_hmem_create(bh_syscall_ctx_t *ctx) {
    bh_hmem_create_args_t args;
    bh_status_t st = bh_copy_from_user(&args, (const void *)ctx->regs.arg[0], sizeof(args));
    if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

    if (args.struct_size != sizeof(bh_hmem_create_args_t)) {
        return bh_op_result_kstatus(K_ERR_INVALID_ARG);
    }

    if (args.resource_cap != 0) {
        bh_process_object_t target;
        kstatus_t kst = bh_syscall_cap_lookup_process(ctx, args.resource_cap, CAP_RIGHT_RESOURCE_ALLOC, &target);
        if (kst != K_OK) return bh_op_result_kstatus(kst);
    }

    uint32_t core = hal_cpu_get_id();
    bh_hmem_t *object = NULL;
    kstatus_t kst = bh_hmem_create_object(&args.desc, args.rights, core, &object);
    if (kst != K_OK) return bh_op_result_kstatus(kst);

    capability_table_t *table = (capability_table_t *)ctx->process->security_sandbox_ctx;
    if (!table) {
        bh_hmem_destroy_object(object);
        return bh_op_result_kstatus(K_ERR_DENIED);
    }

    cap_rights_mask_t cap_rights = hmem_rights_to_cap_rights(args.rights);
    uint32_t cap_id = 0;
    int grant_res = cap_table_grant(table, CAP_TYPE_HMEM, (uint64_t)(uintptr_t)object, cap_rights, &cap_id);
    if (grant_res != 0) {
        bh_hmem_destroy_object(object);
        return bh_op_result_kstatus(K_ERR_NO_RESOURCES);
    }

    args.out_handle = (bharat_hmem_handle_t)cap_id;
    st = bh_copy_to_user((void *)ctx->regs.arg[0], &args, sizeof(args));
    if (st != BH_OK) {
        cap_table_revoke(table, cap_id);
        bh_hmem_destroy_object(object);
        return bh_op_result_kstatus(bh_status_to_kstatus(st));
    }

    return bh_op_result_kstatus(K_OK);
}

bh_operation_result_t bh_sys_hmem_destroy(bh_syscall_ctx_t *ctx) {
    uint32_t cap_id = (uint32_t)ctx->regs.arg[0];
    if (cap_id == 0) return bh_op_result_kstatus(K_ERR_INVALID_ARG);

    bh_hmem_t *object = NULL;
    kstatus_t kst = bh_syscall_cap_lookup_hmem(ctx, cap_id, CAP_RIGHT_HMEM_DESTROY | BH_CAP_RIGHT_DESTROY, &object);
    if (kst != K_OK) return bh_op_result_kstatus(kst);

    capability_table_t *table = (capability_table_t *)ctx->process->security_sandbox_ctx;
    if (table) {
        cap_table_revoke(table, cap_id);
    }

    return bh_op_result_kstatus(bh_hmem_destroy_object(object));
}

bh_operation_result_t bh_sys_hmem_query(bh_syscall_ctx_t *ctx) {
    bh_hmem_query_args_t args;
    bh_status_t st = bh_copy_from_user(&args, (const void *)ctx->regs.arg[0], sizeof(args));
    if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

    if (args.struct_size != sizeof(bh_hmem_query_args_t)) {
        return bh_op_result_kstatus(K_ERR_INVALID_ARG);
    }

    bh_hmem_t *object = NULL;
    kstatus_t kst = bh_syscall_cap_lookup_hmem(ctx, (uint32_t)args.hmem_cap, CAP_RIGHT_QUERY | BH_CAP_RIGHT_QUERY, &object);
    if (kst != K_OK) return bh_op_result_kstatus(kst);

    memset(&args.out_info, 0, sizeof(args.out_info));
    args.out_info.version = 1U;
    args.out_info.struct_size = sizeof(args.out_info);
    args.out_info.size = object->size;
    args.out_info.alignment = object->alignment;
    args.out_info.usage_flags = object->usage_flags;
    args.out_info.property_flags = object->property_flags;
    args.out_info.content_generation = object->content_generation;
    args.out_info.preferred_domain = object->preferred_domain;
    args.out_info.cpu_map_count = object->cpu_map_count;

    st = bh_copy_to_user((void *)ctx->regs.arg[0], &args, sizeof(args));
    return bh_op_result_kstatus(bh_status_to_kstatus(st));
}

bh_operation_result_t bh_sys_hmem_map_cpu(bh_syscall_ctx_t *ctx) {
    bh_hmem_map_cpu_args_t args;
    bh_status_t st = bh_copy_from_user(&args, (const void *)ctx->regs.arg[0], sizeof(args));
    if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

    if (args.struct_size != sizeof(bh_hmem_map_cpu_args_t)) {
        return bh_op_result_kstatus(K_ERR_INVALID_ARG);
    }

    if (args.access == 0U ||
        (args.access & ~(BH_HMEM_ACCESS_READ | BH_HMEM_ACCESS_WRITE | BH_HMEM_ACCESS_EXECUTE)) != 0U) {
        return bh_op_result_kstatus(K_ERR_INVALID_ARG);
    }

    if ((args.access & BH_HMEM_ACCESS_WRITE) != 0U && (args.access & BH_HMEM_ACCESS_EXECUTE) != 0U) {
        return bh_op_result_kstatus(K_ERR_DENIED);
    }

    cap_rights_mask_t req_rights = CAP_RIGHT_HMEM_MAP_CPU | BH_CAP_RIGHT_MAP;
    if (args.access & BH_HMEM_ACCESS_READ) {
        req_rights |= (CAP_RIGHT_READ | BH_CAP_RIGHT_READ);
    }
    if (args.access & BH_HMEM_ACCESS_WRITE) {
        req_rights |= (CAP_RIGHT_WRITE | BH_CAP_RIGHT_WRITE);
    }

    bh_hmem_t *object = NULL;
    kstatus_t kst = bh_syscall_cap_lookup_hmem(ctx, (uint32_t)args.hmem_cap, req_rights, &object);
    if (kst != K_OK) return bh_op_result_kstatus(kst);

    spin_lock(&object->lock);
    if (object->cpu_map_count != 0U) {
        spin_unlock(&object->lock);
        return bh_op_result_kstatus(K_ERR_BUSY);
    }

    address_space_t *aspace = ctx->process ? ctx->process->addr_space : NULL;
    if (!aspace || !aspace->prot_domain) {
        spin_unlock(&object->lock);
        return bh_op_result_kstatus(K_ERR_BAD_STATE);
    }

    size_t aligned_len = (size_t)((object->size + PAGE_SIZE - 1U) & ~(PAGE_SIZE - 1U));
    uintptr_t cand_va = 0x40000000ULL;
    if (cand_va < aspace->user_base) cand_va = aspace->user_base;

    spin_lock(&aspace->lock);
    vm_region_t *r = aspace->regions;
    while (r != NULL) {
        if (cand_va + aligned_len <= r->base) {
            break;
        }
        if (cand_va < r->base + r->length) {
            cand_va = (r->base + r->length + PAGE_SIZE - 1U) & ~(PAGE_SIZE - 1U);
        }
        r = r->next;
    }
    if (aspace->user_limit != 0 && cand_va + aligned_len > aspace->user_limit) {
        spin_unlock(&aspace->lock);
        spin_unlock(&object->lock);
        return bh_op_result_kstatus(K_ERR_NO_MEMORY);
    }
    spin_unlock(&aspace->lock);

    uint32_t prot_flags = VM_PROT_USER;
    if (args.access & BH_HMEM_ACCESS_READ) prot_flags |= VM_PROT_READ;
    if (args.access & BH_HMEM_ACCESS_WRITE) prot_flags |= VM_PROT_WRITE;
    if (args.access & BH_HMEM_ACCESS_EXECUTE) prot_flags |= VM_PROT_EXEC;

    int map_ret = prot_domain_map_region(aspace->prot_domain, cand_va, object->pmm_block.phys_addr, aligned_len, prot_flags);
    if (map_ret != 0) {
        spin_unlock(&object->lock);
        return bh_op_result_kstatus(K_ERR_NO_MEMORY);
    }

    vm_region_t *out_region = NULL;
    int attach_ret = aspace_region_attach(aspace, cand_va, aligned_len, prot_flags, 0, VM_INHERIT_COPY_META, object->vm_obj, 0, &out_region);
    if (attach_ret != 0) {
        prot_domain_unmap_region(aspace->prot_domain, cand_va, aligned_len);
        spin_unlock(&object->lock);
        return bh_op_result_kstatus((kstatus_t)attach_ret);
    }

    object->mapped_user_va = cand_va;
    object->mapped_aspace = aspace;
    object->cpu_map_count = 1U;
    spin_unlock(&object->lock);

    args.out_addr = (uint64_t)cand_va;
    st = bh_copy_to_user((void *)ctx->regs.arg[0], &args, sizeof(args));
    if (st != BH_OK) {
        spin_lock(&object->lock);
        aspace_region_detach(aspace, cand_va);
        prot_domain_unmap_region(aspace->prot_domain, cand_va, aligned_len);
        object->mapped_user_va = 0;
        object->mapped_aspace = NULL;
        object->cpu_map_count = 0;
        spin_unlock(&object->lock);
        return bh_op_result_kstatus(bh_status_to_kstatus(st));
    }

    return bh_op_result_kstatus(K_OK);
}

bh_operation_result_t bh_sys_hmem_unmap_cpu(bh_syscall_ctx_t *ctx) {
    bh_hmem_unmap_cpu_args_t args;
    bh_status_t st = bh_copy_from_user(&args, (const void *)ctx->regs.arg[0], sizeof(args));
    if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

    if (args.struct_size != sizeof(bh_hmem_unmap_cpu_args_t)) {
        return bh_op_result_kstatus(K_ERR_INVALID_ARG);
    }

    bh_hmem_t *object = NULL;
    kstatus_t kst = bh_syscall_cap_lookup_hmem(ctx, (uint32_t)args.hmem_cap, CAP_RIGHT_HMEM_MAP_CPU | BH_CAP_RIGHT_UNMAP, &object);
    if (kst != K_OK) return bh_op_result_kstatus(kst);

    spin_lock(&object->lock);
    if (object->cpu_map_count == 0U || object->mapped_user_va == 0) {
        spin_unlock(&object->lock);
        return bh_op_result_kstatus(K_ERR_BAD_STATE);
    }

    address_space_t *aspace = object->mapped_aspace;
    uintptr_t va = object->mapped_user_va;
    size_t aligned_len = (size_t)((object->size + PAGE_SIZE - 1U) & ~(PAGE_SIZE - 1U));

    if (aspace && aspace->prot_domain) {
        prot_domain_unmap_region(aspace->prot_domain, va, aligned_len);
        aspace_region_detach(aspace, va);
        tlb_invalidate_all(aspace, va, aligned_len, TLB_INV_PAGE);
    }

    object->mapped_user_va = 0;
    object->mapped_aspace = NULL;
    object->cpu_map_count = 0;
    object->content_generation++;
    spin_unlock(&object->lock);

    return bh_op_result_kstatus(K_OK);
}

bh_operation_result_t bh_sys_hmem_sync_for_cpu(bh_syscall_ctx_t *ctx) {
    bh_hmem_sync_args_t args;
    bh_status_t st = bh_copy_from_user(&args, (const void *)ctx->regs.arg[0], sizeof(args));
    if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

    if (args.struct_size != sizeof(bh_hmem_sync_args_t)) {
        return bh_op_result_kstatus(K_ERR_INVALID_ARG);
    }

    bh_hmem_t *object = NULL;
    kstatus_t kst = bh_syscall_cap_lookup_hmem(ctx, (uint32_t)args.hmem_cap, CAP_RIGHT_READ | BH_CAP_RIGHT_READ, &object);
    if (kst != K_OK) return bh_op_result_kstatus(kst);

    if (args.length == 0U || args.offset > object->size || args.length > object->size - args.offset) {
        return bh_op_result_kstatus(K_ERR_INVALID_ARG);
    }

    if ((object->property_flags & BH_HMEM_PROP_CPU_COHERENT) != 0U) {
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        return bh_op_result_kstatus(K_OK);
    }

    hal_hmem_status_t hst = hal_hmem_sync_range((uintptr_t)object->kernel_mapping + (uintptr_t)args.offset,
                                                (size_t)args.length, HAL_HMEM_SYNC_TO_CPU);
    return bh_op_result_kstatus(hst == HAL_HMEM_OK ? K_OK : K_ERR_BAD_STATE);
}

bh_operation_result_t bh_sys_hmem_sync_for_device(bh_syscall_ctx_t *ctx) {
    bh_hmem_sync_args_t args;
    bh_status_t st = bh_copy_from_user(&args, (const void *)ctx->regs.arg[0], sizeof(args));
    if (st != BH_OK) return bh_op_result_kstatus(bh_status_to_kstatus(st));

    if (args.struct_size != sizeof(bh_hmem_sync_args_t)) {
        return bh_op_result_kstatus(K_ERR_INVALID_ARG);
    }

    bh_hmem_t *object = NULL;
    kstatus_t kst = bh_syscall_cap_lookup_hmem(ctx, (uint32_t)args.hmem_cap, CAP_RIGHT_WRITE | BH_CAP_RIGHT_WRITE, &object);
    if (kst != K_OK) return bh_op_result_kstatus(kst);

    if (args.length == 0U || args.offset > object->size || args.length > object->size - args.offset) {
        return bh_op_result_kstatus(K_ERR_INVALID_ARG);
    }

    if ((object->property_flags & BH_HMEM_PROP_CPU_COHERENT) != 0U) {
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        return bh_op_result_kstatus(K_OK);
    }

    hal_hmem_status_t hst = hal_hmem_sync_range((uintptr_t)object->kernel_mapping + (uintptr_t)args.offset,
                                                (size_t)args.length, HAL_HMEM_SYNC_TO_DEVICE);
    return bh_op_result_kstatus(hst == HAL_HMEM_OK ? K_OK : K_ERR_BAD_STATE);
}

#endif
