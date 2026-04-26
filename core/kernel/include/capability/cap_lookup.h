#ifndef BHARAT_KERNEL_CAP_LOOKUP_H
#define BHARAT_KERNEL_CAP_LOOKUP_H

#include "trap/syscall_context.h"
#include "capability.h"

/**
 * Syscall-friendly capability lookup wrappers.
 * These helpers use the syscall context to resolve capabilities safely.
 */

static inline kstatus_t bh_syscall_cap_lookup_thread(bh_syscall_ctx_t *ctx,
                                                    uintptr_t cap_id,
                                                    uint64_t required_rights,
                                                    bh_thread_object_t *out) {
    if (!ctx || !ctx->process || !ctx->process->security_sandbox_ctx) {
        return K_ERR_DENIED;
    }
    return cap_lookup_thread((capability_table_t *)ctx->process->security_sandbox_ctx,
                             (uint32_t)cap_id, required_rights, out);
}

static inline kstatus_t bh_syscall_cap_lookup_memory(bh_syscall_ctx_t *ctx,
                                                    uintptr_t cap_id,
                                                    uint64_t required_rights,
                                                    bh_memory_object_t *out) {
    if (!ctx || !ctx->process || !ctx->process->security_sandbox_ctx) {
        return K_ERR_DENIED;
    }
    return cap_lookup_memory((capability_table_t *)ctx->process->security_sandbox_ctx,
                             (uint32_t)cap_id, required_rights, out);
}

static inline kstatus_t bh_syscall_cap_lookup_process(bh_syscall_ctx_t *ctx,
                                                     uintptr_t cap_id,
                                                     uint64_t required_rights,
                                                     bh_process_object_t *out) {
    if (!ctx || !ctx->process || !ctx->process->security_sandbox_ctx) {
        return K_ERR_DENIED;
    }
    return cap_lookup_process((capability_table_t *)ctx->process->security_sandbox_ctx,
                              (uint32_t)cap_id, required_rights, out);
}

static inline kstatus_t bh_syscall_cap_lookup_endpoint(bh_syscall_ctx_t *ctx,
                                                      uintptr_t cap_id,
                                                      uint64_t required_rights,
                                                      bh_endpoint_object_t *out) {
    if (!ctx || !ctx->process || !ctx->process->security_sandbox_ctx) {
        return K_ERR_DENIED;
    }
    return cap_lookup_endpoint((capability_table_t *)ctx->process->security_sandbox_ctx,
                               (uint32_t)cap_id, required_rights, out);
}

static inline kstatus_t bh_syscall_cap_lookup_fault_domain(bh_syscall_ctx_t *ctx,
                                                           uintptr_t cap_id,
                                                           uint64_t required_rights,
                                                           void **out_ref) {
    if (!ctx || !ctx->process || !ctx->process->security_sandbox_ctx) {
        return K_ERR_DENIED;
    }
    capability_entry_t entry;
    kstatus_t st = cap_table_lookup((capability_table_t *)ctx->process->security_sandbox_ctx,
                                    (uint32_t)cap_id, CAP_TYPE_PROCESS, required_rights, &entry);
    if (st == K_OK) {
        if (entry.state != CAP_STATE_LIVE) return K_ERR_DENIED;
        if (out_ref) *out_ref = (void *)entry.object_ref;
    }
    return st;
}

static inline kstatus_t bh_syscall_cap_lookup_sched_control(bh_syscall_ctx_t *ctx,
                                                            uintptr_t cap_id,
                                                            uint64_t required_rights,
                                                            void **out_ref) {
    if (!ctx || !ctx->process || !ctx->process->security_sandbox_ctx) {
        return K_ERR_DENIED;
    }
    capability_entry_t entry;
    kstatus_t st = cap_table_lookup((capability_table_t *)ctx->process->security_sandbox_ctx,
                                    (uint32_t)cap_id, CAP_TYPE_SCHED, required_rights, &entry);
    if (st == K_OK) {
        if (entry.state != CAP_STATE_LIVE) return K_ERR_DENIED;
        if (out_ref) *out_ref = (void *)entry.object_ref;
    }
    return st;
}

#endif /* BHARAT_KERNEL_CAP_LOOKUP_H */
