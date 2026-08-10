#ifndef BHARAT_SCHED_CPU_CONTEXT_H
#define BHARAT_SCHED_CPU_CONTEXT_H

#include <stddef.h>
#include <stdint.h>

typedef struct arch_ext_state arch_ext_state_t;

/*
 * Architecture-local saved CPU state. Every scalar slot has the native
 * register width; context-switch assembly consumes C-generated BH_CTX_*
 * offsets rather than duplicating this layout. This object is core-local to
 * the owning thread and is never transferred over a wire boundary.
 */
typedef struct {
    uintptr_t regs[16];
    uintptr_t pc;
    uintptr_t sp;
    uintptr_t fpu_regs[32];
    arch_ext_state_t *ext;
} cpu_context_t;

_Static_assert(offsetof(cpu_context_t, regs) == 0U,
               "cpu_context_t register array must be first");
_Static_assert(offsetof(cpu_context_t, pc) == 16U * sizeof(uintptr_t),
               "cpu_context_t pc offset mismatch");
_Static_assert(offsetof(cpu_context_t, sp) == 17U * sizeof(uintptr_t),
               "cpu_context_t sp offset mismatch");
_Static_assert(offsetof(cpu_context_t, ext) == 50U * sizeof(uintptr_t),
               "cpu_context_t ext-state offset mismatch");

#endif
