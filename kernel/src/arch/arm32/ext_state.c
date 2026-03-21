#include "../../include/arch/arch_ext_state.h"

void arch_ext_state_init(void) {}

bool arch_ext_state_handle_fault(struct kthread *t) {
    (void)t;
    return false;
}
