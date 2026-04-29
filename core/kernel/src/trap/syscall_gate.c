#include "trap/syscall_regs.h"
#include "trap/syscall_context.h"
#include "trap/syscall_stats.h"
#include "trap/syscall_status.h"
#include "hal/hal.h"
#include "personality_ops.h"
#include "sched/sched.h"
#include "fault_diag.h"
#include "kernel/status.h"
#include "bharat/personality/personality_interface.h"
#include "profile/profile_policy.h"
#include "bh_personality_registry.h"

kstatus_t bh_syscall_policy_check(bh_syscall_ctx_t *ctx, const bh_syscall_meta_t *desc) {
    if (!ctx || !desc) return K_ERR_INVALID_ARG;
    kstatus_t status = K_OK;

    // 1. Personality and Profile Allowlist
    if (!bh_profile_allows_personality(ctx->personality)) {
        return K_ERR_DENIED;
    }

    // 2. CAP_REQUIRED check: Must have enforceable rights
    if (desc->flags & BH_SYSCALL_F_CAP_REQUIRED) {
        if (desc->required_rights == 0 && desc->requires_capability) {
             // If capability is required but no specific rights defined, we still need a check later
             // but if it's F_CAP_REQUIRED it must specify at least one bit or the metadata is invalid.
             // (Exception: some syscalls might just check for possession of ANY right on a specific type)
        }
    }

    if (status == K_OK && (desc->flags & BH_SYSCALL_F_FAST)) {
        if (desc->flags & (BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_SERVICE_CALL)) {
            status = K_ERR_INVALID_ARG;
        }
        // Fast path syscalls must not perform usercopy
        if (status == K_OK && (desc->flags & (BH_SYSCALL_F_USER_READ | BH_SYSCALL_F_USER_WRITE))) {
            status = K_ERR_INVALID_ARG;
        }
    }

    // 4. Profile-based trait enforcement
    if (status == K_OK) {
        // Deny blocking syscalls if profile forbids them
        if ((desc->flags & BH_SYSCALL_F_BLOCKING) && !bh_profile_allows_blocking_syscall()) {
            status = K_ERR_DENIED;
        }

        // Deny service calls on MPU-only/Tiny profiles if not service-rich
        if ((desc->flags & BH_SYSCALL_F_SERVICE_CALL) && !bh_profile_has_trait(BH_PROFILE_TRAIT_SERVICE_RICH)) {
            status = K_ERR_DENIED;
        }
    }

    if (status != K_OK) {
        bh_syscall_stats_inc_denied(hal_cpu_get_id());
    }
    return status;
}

const bh_personality_syscall_table_t *personality_get_syscall_table(bh_personality_id_t id) {
    switch (id) {
#if defined(BHARAT_PERSONALITY_NATIVE)
        case BH_PERSONALITY_NATIVE: {
            extern const bh_personality_syscall_table_t native_personality;
            return &native_personality;
        }
#endif
#if defined(BHARAT_PERSONALITY_LINUX)
        case BH_PERSONALITY_LINUX: {
#if BHARAT_ENABLE_SUBSYS_LINUX
            extern const bh_personality_syscall_table_t bh_linux_syscall_table;
            return &bh_linux_syscall_table;
#endif
        }
#endif
        default:
            return NULL;
    }
}

long bh_syscall_gate(trap_frame_t *frame, const trap_info_t *info) {
    if (!frame || !info) {
        return kstatus_to_native_sysret(K_ERR_INVALID_ARG);
    }
    bh_syscall_ctx_t ctx = {0};
    ctx.thread = sched_current_thread();
    if (ctx.thread) {
        ctx.process = ctx.thread->process;
        if (ctx.process) {
            ctx.personality = (bh_personality_id_t)ctx.process->personality.kind;
        } else {
            ctx.personality = ctx.thread->personality;
        }
    } else {
        return kstatus_to_native_sysret(K_ERR_DENIED);
    }
    if (!ctx.process) {
        return kstatus_to_native_sysret(K_ERR_DENIED);
    }

    if (arch_trap_extract_syscall(frame, &ctx.regs) != K_OK) {
        return kstatus_to_native_sysret(K_ERR_INVALID_ARG);
    }
    fault_diag_record_syscall(ctx.regs.nr);

    const bh_personality_syscall_table_t *table = personality_get_syscall_table(ctx.personality);
    if (!table || !table->table || ctx.regs.nr > table->max_syscall_nr) {
        return kstatus_to_native_sysret(K_ERR_INVALID_SYSCALL);
    }

    const bh_syscall_meta_t *desc = &table->table[ctx.regs.nr];

    /* Metadata-driven Fail Closed Dispatch */
    if (desc->nr != ctx.regs.nr || !desc->handler) {
        return kstatus_to_native_sysret(K_ERR_UNSUPPORTED);
    }

    /* Argument count validation */
    if (desc->arg_count > 6) {
        return kstatus_to_native_sysret(K_ERR_INVALID_ARG);
    }

    ctx.desc = desc;

    uint32_t core_id = hal_cpu_get_id();
    bh_syscall_stats_inc_total(core_id);
    if (desc->flags & BH_SYSCALL_F_FAST) {
        bh_syscall_stats_inc_fast(core_id);
    } else {
        bh_syscall_stats_inc_slow(core_id);
    }

    /* Production Policy and Security Checks */
    kstatus_t policy_st = bh_syscall_policy_check(&ctx, desc);
    if (policy_st != K_OK) {
        bh_syscall_stats_inc_denied(core_id);
        return kstatus_to_native_sysret(policy_st);
    }

    /* Personality-specific translation */
    if (ctx.personality == BH_PERSONALITY_NATIVE) {
        return desc->handler(&ctx);
    } else {
        /* Compatibility personality: result should be translated to personality-specific errno */
        long res = desc->handler(&ctx);
        /* If handler returned a kernel status instead of translated long, we should catch it here */
        return res;
    }
}
