#include "trap/syscall_regs.h"
#include "trap/syscall_context.h"
#include "trap/syscall_stats.h"
#include "hal/hal.h"
#include "personality_ops.h"
#include "sched/sched.h"
#include "fault_diag.h"
#include "kernel/status.h"
#include "bharat/personality/personality_interface.h"

kstatus_t bh_syscall_policy_check(bh_syscall_ctx_t *ctx, const bh_syscall_desc_t *desc) {
    if (!ctx || !desc) return K_ERR_INVALID_ARG;

    // 1. Syscall allowed for personality (already checked by table lookup, but for double-check)

    // 2. Syscall allowed for process sandbox/profile

    // 3. CAP_REQUIRED check (Stage 4 will harden)
    // if (desc->flags & BH_SYSCALL_F_CAP_REQUIRED) { ... }

    return K_OK;
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

    // Flag validation
    if (desc->flags & BH_SYSCALL_F_FAST) {
        // Fast path syscalls must not block or jump to services
        if (desc->flags & (BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_SERVICE_CALL)) {
            return kstatus_to_sysret(K_ERR_INVALID_ARG);
        }
        // Fast path syscalls must not perform usercopy (must be explicitly whitelisted if needed later)
        if (desc->flags & (BH_SYSCALL_F_USER_READ | BH_SYSCALL_F_USER_WRITE)) {
            return kstatus_to_sysret(K_ERR_INVALID_ARG);
        }
    }

    // CAP_REQUIRED must have enforceable rights
    if ((desc->flags & BH_SYSCALL_F_CAP_REQUIRED) && desc->required_rights == 0) {
        return kstatus_to_sysret(K_ERR_DENIED);
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
