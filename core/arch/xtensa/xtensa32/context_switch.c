void arch_xtensa32_context_switch(void *from_ctx, void *to_ctx) {
    (void)from_ctx;
    (void)to_ctx;
}

void arch_prepare_initial_context_arg(
    cpu_context_t *ctx,
    arch_thread_entry_arg_t entry,
    void *arg0,
    uintptr_t stack_top)
{
    // Stub
}
