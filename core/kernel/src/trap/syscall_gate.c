#include "trap/syscall_regs.h"
#include "trap/syscall_context.h"
#include "trap/syscall_stats.h"
#include "hal/hal.h"
#include "personality_ops.h"
#include "sched/sched.h"
#include "fault_diag.h"
#include "kernel/status.h"

extern const bh_personality_syscall_table_t *personality_get_syscall_table(bh_personality_id_t id);

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
        // In future, personality should be stored in bh_process_t/bh_thread_t
        ctx.personality = ctx.thread->personality;
    } else {
        // Fallback for early boot or system threads if they ever trigger syscalls
        ctx.personality = BH_PERSONALITY_NATIVE;
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

    // 4. Performance stats
    uint32_t core_id = hal_cpu_get_id();
    bh_syscall_stats_inc_total(core_id);
    if (desc->flags & BH_SYSCALL_F_FAST) {
        bh_syscall_stats_inc_fast(core_id);
    } else {
        bh_syscall_stats_inc_slow(core_id);
    }

    // 5. Generic Policy Hooks (Stage 4 will harden)
    // if (desc->flags & BH_SYSCALL_F_CAP_REQUIRED) { ... }

    // 6. Dispatch
    long rc = desc->handler(&ctx);

    return rc;
}
