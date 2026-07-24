#include "arch/context_switch.h"
#include "arch/arch_ext_state.h"
#include <stddef.h>
#include <stdint.h>

void arch_prepare_initial_context(cpu_context_t* ctx, void (*entry)(void), uint64_t stack_top) {
    (void)ctx;
    (void)entry;
    (void)stack_top;
    // riscv32 context switch is currently a stub
}
