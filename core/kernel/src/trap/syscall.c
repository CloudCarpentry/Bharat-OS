#include "trap/syscall_regs.h"
#include "trap/syscall_context.h"
#include "trap/syscall_stats.h"
#include "hal/hal.h"
#include "personality_ops.h"
#include "sched/sched.h"
#include "fault_diag.h"
#include "kernel/status.h"

extern const bh_personality_syscall_table_t *personality_get_syscall_table(bh_personality_id_t id);

long bh_syscall_gate(trap_frame_t *frame, const trap_info_t *info) {
    bh_syscall_ctx_t ctx = {0};
    ctx.thread = sched_current_thread();
    if (ctx.thread) {
        ctx.process = ctx.thread->process;
    }

    if (arch_trap_extract_syscall(frame, &ctx.regs) != K_OK) {
        return kstatus_to_sysret(K_ERR_INVALID_ARG);
    }

    fault_diag_record_syscall(ctx.regs.nr);

    // Determine personality. For now, we assume native if not specified.
    // In future, this should be part of bh_process_t.
    ctx.personality = BH_PERSONALITY_NATIVE;

    const bh_personality_syscall_table_t *table = personality_get_syscall_table(ctx.personality);
    if (!table || ctx.regs.nr > table->max_syscall_nr) {
        return kstatus_to_sysret(K_ERR_UNSUPPORTED);
    }

    const bh_syscall_desc_t *desc = &table->table[ctx.regs.nr];
    if (!desc->handler) {
        return kstatus_to_sysret(K_ERR_UNSUPPORTED);
    }

    uint32_t core_id = hal_cpu_get_id();
    bh_syscall_stats_inc_total(core_id);
    if (desc->flags & BH_SYSCALL_F_FAST) {
        bh_syscall_stats_inc_fast(core_id);
    } else {
        bh_syscall_stats_inc_slow(core_id);
    }

    // Security checks (simplified for now, stage 4 will harden)
    // if (desc->flags & BH_SYSCALL_F_CAP_REQUIRED) { ... }

    long rc = desc->handler(&ctx);

    return rc;
}

// TODO: Remove this once no longer needed by any personality
long syscall_dispatch(syscall_id_t id, uintptr_t arg0, uintptr_t arg1,
                      uintptr_t arg2, uintptr_t arg3, uintptr_t arg4,
                      uintptr_t arg5) {
    (void)id; (void)arg0; (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    return kstatus_to_sysret(K_ERR_UNSUPPORTED);
}

long trap_dispatch_syscall(trap_frame_t *frame, const trap_info_t *info) {
    long rc = bh_syscall_gate(frame, info);
    arch_trap_set_syscall_return(frame, (uintptr_t)rc);
    return rc;
}
