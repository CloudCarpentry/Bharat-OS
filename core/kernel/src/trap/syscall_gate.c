#include "trap/syscall_regs.h"
#include "trap/syscall_context.h"
#include "trap/syscall_stats.h"
#include "hal/hal.h"
#include "personality_ops.h"
#include "sched/sched.h"
#include "fault_diag.h"
#include "kernel/status.h"
#include "bharat/personality/personality_interface.h"
#include "profile/profile_policy.h"

kstatus_t bh_syscall_policy_check(bh_syscall_ctx_t *ctx, const bh_syscall_desc_t *desc) {
    if (!ctx || !desc) return K_ERR_INVALID_ARG;

    kstatus_t status = K_OK;

    // 1. Personality and Profile Allowlist
    if (!bh_profile_allows_personality(ctx->personality)) {
        return K_ERR_DENIED;
    }

    // 2. CAP_REQUIRED check: Must have enforceable rights
    if (desc->flags & BH_SYSCALL_F_CAP_REQUIRED) {
        if (desc->required_rights == 0) {
            // FAIL CLOSED: CAP_REQUIRED syscall without specified rights is a descriptor bug
            status = K_ERR_DENIED;
        }
    }

    // 3. Fast path constraints
    if (status == K_OK && (desc->flags & BH_SYSCALL_F_FAST)) {
        // Fast path syscalls must not block or jump to services
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
        case BH_PERSONALITY_NATIVE:
            return personality_native_get_table();
        case BH_PERSONALITY_LINUX:
            return personality_linux_get_table();
        case BH_PERSONALITY_ANDROID:
            return personality_android_get_table();
        case BH_PERSONALITY_WINDOWS:
            return personality_windows_get_table();
        default:
            return NULL;
    }
}

/**
 * bh_syscall_gate: The common secure syscall gate substrate.
 *
 * This function is personality-neutral and performs:
 * 1. Syscall extraction (arch-specific)
 * 2. Context construction
 * 3. Personality-specific table lookup
 * 4. Bounds and descriptor validation
 * 5. Performance stats accounting
 * 6. Dispatch to handler
 *
 * Return value is the syscall result (normalized to personality-specific error space).
 *
 * CONTRACT: This function MUST NOT call arch_trap_set_syscall_return().
 * Return register ownership belongs to the top-level trap_dispatch_syscall().
 */
long bh_syscall_gate(trap_frame_t *frame, const trap_info_t *info) {
    if (!frame || !info) {
        return kstatus_to_sysret(K_ERR_INVALID_ARG);
    }

    bh_syscall_ctx_t ctx = {0};
    ctx.thread = sched_current_thread();
    if (ctx.thread) {
        ctx.process = ctx.thread->process;
        ctx.personality = ctx.thread->personality;
    } else {
        // FAIL CLOSED: No user context for userspace syscall
        return kstatus_to_sysret(K_ERR_DENIED);
    }

    if (!ctx.process) {
        return kstatus_to_sysret(K_ERR_DENIED);
    }

    // 1. Arch-specific extraction
    if (arch_trap_extract_syscall(frame, &ctx.regs) != K_OK) {
        return kstatus_to_sysret(K_ERR_INVALID_ARG);
    }

    fault_diag_record_syscall(ctx.regs.nr);

    // 2. Personality-specific lookup
    const bh_personality_syscall_table_t *table = personality_get_syscall_table(ctx.personality);
    if (!table || !table->table || ctx.regs.nr > table->max_syscall_nr) {
        return kstatus_to_sysret(K_ERR_UNSUPPORTED);
    }

    // 3. Descriptor validation
    const bh_syscall_desc_t *desc = &table->table[ctx.regs.nr];
    if (desc->nr != ctx.regs.nr || !desc->handler) {
        return kstatus_to_sysret(K_ERR_UNSUPPORTED);
    }

    if (desc->arg_count > 6) {
        return kstatus_to_sysret(K_ERR_INVALID_ARG);
    }


    ctx.desc = desc;

    // 4. Performance stats
    uint32_t core_id = hal_cpu_get_id();
    bh_syscall_stats_inc_total(core_id);
    if (desc->flags & BH_SYSCALL_F_FAST) {
        bh_syscall_stats_inc_fast(core_id);
    } else {
        bh_syscall_stats_inc_slow(core_id);
    }

    // 5. Generic Policy Hooks
    kstatus_t policy_st = bh_syscall_policy_check(&ctx, desc);
    if (policy_st != K_OK) {
        bh_syscall_stats_inc_denied(core_id);
        return kstatus_to_sysret(policy_st);
    }

    // 6. Dispatch
    long rc = desc->handler(&ctx);

    return rc;
}
