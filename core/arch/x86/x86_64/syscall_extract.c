#include "trap/syscall_regs.h"
#include "trap/syscall_context.h"
#include "hal/hal.h"
#include "sched/sched.h"

#define X86_RFLAGS_RESERVED_1 (1ULL << 1)
#define X86_RFLAGS_IOPL_MASK  (3ULL << 12)

static bool is_canonical(uintptr_t addr) {
    uintptr_t sign_bit = (addr >> 47) & 1;
    if (sign_bit) {
        return (addr >> 48) == 0xFFFF;
    } else {
        return (addr >> 48) == 0;
    }
}

void x86_syscall_validate_return(bh_syscall_return_context_t *ret) {
    if (!ret) return;

    bool valid = true;

    // 1. Validate canonical user RIP
    if (!is_canonical(ret->pc) || ret->pc >= 0x8000000000000000ULL) {
        valid = false;
    }

    // 2. Validate canonical user RSP
    if (!is_canonical(ret->sp) || ret->sp >= 0x8000000000000000ULL) {
        valid = false;
    }

    // 3. Validate RFLAGS (Status)
    // Must have bit 1 set. IOPL should not be elevated for normal user (CPL3).
    if ((ret->status & X86_RFLAGS_RESERVED_1) == 0) {
        valid = false;
    }
    // Deny IOPL > 0 for standard user mode to prevent I/O privilege escalation.
    if ((ret->status & X86_RFLAGS_IOPL_MASK) != 0) {
        valid = false;
    }

    // 4. Origin must be user
    if (ret->origin != TRAP_ORIGIN_USER) {
        valid = false;
    }

    if (!valid) {
        ret->disposition = BH_SYSCALL_RETURN_FAULT;
        bh_thread_t *thread = sched_current_thread();
        if (thread) {
            thread_raise_fault(thread, THREAD_FAULT_SEGV);
            // If the thread faults, we must not return via SYSRET/IRET to invalid context.
            // The thread_raise_fault will put the thread into a fault state.
            // When we return to assembly, it will proceed to SYSRET, but the thread
            // shouldn't actually execute user code anymore if the scheduler handles faults immediately,
            // or we might need to block. However, thread_raise_fault marks it faulted.
            // In Bharat-OS, thread_raise_fault likely calls sched_reschedule internally
            // or at trap exit. We must make sure not to SYSRET to bad RIP.
            // By resetting pc and sp to a safe loop or letting the scheduler pick another thread,
            // we avoid the GPF. Since this is an unrecoverable syscall return context,
            // we will loop here and call schedule.

            // For now, raising fault is the requirement.
            // To be absolutely safe from IRET/SYSRET #GP, zero out the context.
            ret->pc = 0;
            ret->sp = 0;
            ret->status = X86_RFLAGS_RESERVED_1;

            sched_reschedule();
        }
    }
}

bool arch_trap_status_interrupt_enabled(const trap_frame_t *frame) {
    if (!frame) return false;
    return (frame->status & (1ULL << 9)) != 0;
}

bool arch_trap_is_syscall(const trap_frame_t *frame) {
    if (!frame) return false;
    /*
     * x86_64:
     * - Transitional path using INT 0x80 (cause 0x80)
     * - Production path using SYSCALL (pseudo-cause 0x100)
     */
    return (frame->cause == 0x80U || frame->cause == 0x100U);
}

kstatus_t arch_trap_extract_syscall(const trap_frame_t *frame, bh_syscall_regs_t *out) {
    if (!frame || !out) return K_ERR_INVALID_ARG;

    /*
     * x86_64 Syscall ABI (Transitional INT 0x80 mapping):
     * nr:   rax
     * args: rdi, rsi, rdx, r10, r8, r9
     * Note: trap_entry.S maps rax to gpr[0], rdi to gpr[1], etc.
     */
    out->nr     = frame->gpr[0]; // rax
    out->arg[0] = frame->gpr[1]; // rdi
    out->arg[1] = frame->gpr[2]; // rsi
    out->arg[2] = frame->gpr[3]; // rdx
    out->arg[3] = frame->gpr[4]; // r10
    out->arg[4] = frame->gpr[5]; // r8
    out->arg[5] = frame->gpr[6]; // r9

    return K_OK;
}

void arch_trap_set_syscall_return(trap_frame_t *frame, uintptr_t value) {
    if (!frame) return;
    frame->gpr[0] = value; // rax
}
