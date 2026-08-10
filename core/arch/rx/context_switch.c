void arch_rx_context_switch(void *from, void *to) {
    /* RX context switch: Save/Restore R1-R15, FPSW, etc. */
}

void arch_prepare_initial_context_arg(
    cpu_context_t *ctx,
    arch_thread_entry_arg_t entry,
    void *arg0,
    uintptr_t stack_top)
{
    // Stub
}
