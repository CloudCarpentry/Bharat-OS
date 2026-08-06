void arch_tricore_context_switch(void *from, void *to) {
    /* TriCore uses hardware-managed context switching via SVLCX/RSLCX and CSAs */
}

void arch_prepare_initial_context_arg(
    cpu_context_t *ctx,
    arch_thread_entry_arg_t entry,
    void *arg0,
    uintptr_t stack_top)
{
    // Stub
}
