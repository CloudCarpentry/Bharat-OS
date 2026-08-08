#ifndef BHARAT_SYSCALL_TEST_H
#define BHARAT_SYSCALL_TEST_H

#include "trap/syscall_context.h"
#include <stdbool.h>

/* Internal test contract — never UAPI. */
typedef enum {
    BH_SYSCALL_TEST_RETURN_NONE = 0,
    BH_SYSCALL_TEST_RETURN_BAD_PC,
    BH_SYSCALL_TEST_RETURN_BAD_SP,
    BH_SYSCALL_TEST_RETURN_BAD_STATUS,
} bh_syscall_test_return_fault_t;

#if defined(BHARAT_ENABLE_TEST_HOOKS)

void bh_syscall_test_arm_return_fault(bh_syscall_test_return_fault_t fault);

bool bh_syscall_test_apply_return_fault(bh_syscall_return_context_t *ret);

#endif /* BHARAT_ENABLE_TEST_HOOKS */

#endif /* BHARAT_SYSCALL_TEST_H */
