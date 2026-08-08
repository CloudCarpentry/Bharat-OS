.section .text
.global _context_switch
_context_switch:
    b .

.global arch_bh_thread_start_trampoline
.type arch_bh_thread_start_trampoline, %function
arch_bh_thread_start_trampoline:
    /* r4 holds the true entry point, setup by arch_prepare_initial_context */
    /* r0 holds arg0 */
    blx r4
    b sched_thread_exit_trampoline
