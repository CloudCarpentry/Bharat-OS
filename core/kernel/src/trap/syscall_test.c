#include "trap/syscall_test.h"
#include <bharat/cpu_local.h>
#include "hal/hal.h"

#if defined(BHARAT_ENABLE_TEST_HOOKS)

typedef struct {
    uint32_t armed;
    bh_syscall_test_return_fault_t kind;
} bh_syscall_test_state_t;

static bh_syscall_test_state_t g_test_state[MAX_CPUS];

void bh_syscall_test_arm_return_fault(bh_syscall_test_return_fault_t fault) {
    uint32_t cpu_id = hal_cpu_get_id();
    if (cpu_id < MAX_CPUS) {
        g_test_state[cpu_id].kind = fault;
        g_test_state[cpu_id].armed = 1;
    }
}

bool bh_syscall_test_apply_return_fault(bh_syscall_return_context_t *ret) {
    if (!ret) return false;

    uint32_t cpu_id = hal_cpu_get_id();
    if (cpu_id >= MAX_CPUS) return false;

    bh_syscall_test_state_t *state = &g_test_state[cpu_id];

    if (!state->armed) return false;

    state->armed = 0; /* consume before mutation */

    switch (state->kind) {
    case BH_SYSCALL_TEST_RETURN_BAD_PC:
        ret->pc = 0x0000800000000000ULL; // Noncanonical
        break;

    case BH_SYSCALL_TEST_RETURN_BAD_SP:
        ret->sp = 0x0000800000000000ULL; // Noncanonical
        break;

    case BH_SYSCALL_TEST_RETURN_BAD_STATUS:
        // Set an invalid status, e.g., elevated IOPL
        ret->status |= (3ULL << 12);
        break;

    default:
        return false;
    }

    return true;
}

#endif /* BHARAT_ENABLE_TEST_HOOKS */
