#ifndef BHARAT_KERNEL_TRAP_REASON_H
#define BHARAT_KERNEL_TRAP_REASON_H

#include <stdint.h>

/*
 * Bharat-OS Arch-Neutral Trap Reasons
 * High 8 bits = class, low 24 bits = specific reason.
 */

typedef uint32_t trap_reason_t;

#define TRAP_CLASS_SYNC      (1u << 24)
#define TRAP_CLASS_ASYNC     (2u << 24)
#define TRAP_CLASS_FAULT     (3u << 24)
#define TRAP_CLASS_SYSTEM    (4u << 24)

/* ── System / Architected ── */
#define TRAP_SYSCALL            (TRAP_CLASS_SYSTEM | 0u)

/* ── Faults ── */
#define TRAP_PAGE_FAULT_READ    (TRAP_CLASS_FAULT  | 1u)
#define TRAP_PAGE_FAULT_WRITE   (TRAP_CLASS_FAULT  | 2u)
#define TRAP_PAGE_FAULT_EXEC    (TRAP_CLASS_FAULT  | 3u)
#define TRAP_ILLEGAL_INSN       (TRAP_CLASS_FAULT  | 4u)
#define TRAP_DIVIDE_BY_ZERO     (TRAP_CLASS_FAULT  | 5u)
#define TRAP_ALIGNMENT_FAULT    (TRAP_CLASS_FAULT  | 6u)
#define TRAP_SECURITY_VIOLATION (TRAP_CLASS_FAULT  | 7u)
#define TRAP_MACHINE_CHECK      (TRAP_CLASS_FAULT  | 8u)

/* ── Async Events ── */
#define TRAP_TIMER              (TRAP_CLASS_ASYNC  | 1u)
#define TRAP_IRQ_EXTERNAL       (TRAP_CLASS_ASYNC  | 2u)
#define TRAP_IPI                (TRAP_CLASS_ASYNC  | 3u)

#endif // BHARAT_KERNEL_TRAP_REASON_H
