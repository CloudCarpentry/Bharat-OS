/**
 * ARC32 Context Switch Implementation
 *
 * ARC registers to save:
 * r0-r12 (GPRs)
 * r13 (fp), r14 (sp), r15 (gp)
 * r25 (jl), r26 (gp), r30 (pcl)
 * blink (r31)
 * status32
 */

struct arc32_context {
    uint32_t r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12;
    uint32_t fp; // r13
    uint32_t sp; // r14
    uint32_t gp; // r15
    uint32_t blink; // r31
    uint32_t status32;
};

void arch_arc32_context_switch(void *from_ctx, void *to_ctx) {
    if (!to_ctx) return;

    // In a real implementation, this would be in assembly
    // to properly save and restore all registers including status32.
    // This C stub represents the logical transition.
}

void arch_prepare_initial_context_arg(
    cpu_context_t *ctx,
    arch_thread_entry_arg_t entry,
    void *arg0,
    uintptr_t stack_top)
{
    // Stub
}
