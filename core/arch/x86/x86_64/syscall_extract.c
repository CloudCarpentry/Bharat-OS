#include "trap/syscall_regs.h"
#include "trap/syscall_context.h"
#include "arch/x86_syscall_return.h"
#include <stddef.h>

#define X86_RFLAGS_RESERVED_1 (1ULL << 1)
#define X86_RFLAGS_IOPL_MASK  (3ULL << 12)

_Static_assert(offsetof(bh_syscall_return_context_t, pc) == X86_SYSCALL_RET_PC_OFFSET,
               "x86 syscall return pc layout drift");
_Static_assert(offsetof(bh_syscall_return_context_t, sp) == X86_SYSCALL_RET_SP_OFFSET,
               "x86 syscall return sp layout drift");
_Static_assert(offsetof(bh_syscall_return_context_t, status) == X86_SYSCALL_RET_STATUS_OFFSET,
               "x86 syscall return status layout drift");
_Static_assert(offsetof(bh_syscall_return_context_t, result) == X86_SYSCALL_RET_RESULT_OFFSET,
               "x86 syscall return result layout drift");
_Static_assert(offsetof(bh_syscall_return_context_t, origin) == X86_SYSCALL_RET_ORIGIN_OFFSET,
               "x86 syscall return origin layout drift");
_Static_assert(offsetof(bh_syscall_return_context_t, flags) == X86_SYSCALL_RET_FLAGS_OFFSET,
               "x86 syscall return flags layout drift");
_Static_assert(offsetof(bh_syscall_return_context_t, disposition) == X86_SYSCALL_RET_DISPOSITION_OFFSET,
               "x86 syscall return disposition layout drift");
_Static_assert(sizeof(bh_syscall_return_context_t) == X86_SYSCALL_RET_CONTEXT_SIZE,
               "x86 syscall return context size drift");
_Static_assert(BH_SYSCALL_RETURN_USER == X86_SYSCALL_RETURN_USER_VALUE,
               "x86 syscall USER disposition encoding drift");

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
