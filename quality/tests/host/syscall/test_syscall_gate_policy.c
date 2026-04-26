#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "trap/syscall_context.h"
#include "kernel/status.h"

// Stubs for functions called by bh_syscall_policy_check
void bh_syscall_stats_inc_denied(uint32_t core_id) { (void)core_id; }
uint32_t hal_cpu_get_id(void) { return 0; }

// Minimal implementation matching core/kernel/src/trap/syscall_gate.c
kstatus_t bh_syscall_policy_check(bh_syscall_ctx_t *ctx, const bh_syscall_desc_t *desc) {
    if (!ctx || !desc) return K_ERR_INVALID_ARG;

    kstatus_t status = K_OK;

    if (desc->flags & BH_SYSCALL_F_CAP_REQUIRED) {
        if (desc->required_rights == 0) {
            status = K_ERR_DENIED;
        }
    }

    if (status == K_OK && (desc->flags & BH_SYSCALL_F_FAST)) {
        if (desc->flags & (BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_SERVICE_CALL)) {
            status = K_ERR_INVALID_ARG;
        }
        if (status == K_OK && (desc->flags & (BH_SYSCALL_F_USER_READ | BH_SYSCALL_F_USER_WRITE))) {
            status = K_ERR_INVALID_ARG;
        }
    }

    if (status != K_OK) {
        bh_syscall_stats_inc_denied(hal_cpu_get_id());
    }

    return status;
}

void test_policy_enforcement(void) {
    printf("Testing syscall policy enforcement...\n");
    bh_syscall_ctx_t ctx = {0};
    bh_syscall_desc_t desc = {0};

    // Test CAP_REQUIRED without rights (Failure)
    desc.flags = BH_SYSCALL_F_CAP_REQUIRED;
    desc.required_rights = 0;
    assert(bh_syscall_policy_check(&ctx, &desc) == K_ERR_DENIED);

    // Test CAP_REQUIRED with rights (Success)
    desc.required_rights = 1;
    assert(bh_syscall_policy_check(&ctx, &desc) == K_OK);

    // Test FAST + BLOCKING (Failure)
    desc.flags = BH_SYSCALL_F_FAST | BH_SYSCALL_F_BLOCKING;
    assert(bh_syscall_policy_check(&ctx, &desc) == K_ERR_INVALID_ARG);

    // Test FAST + SERVICE_CALL (Failure)
    desc.flags = BH_SYSCALL_F_FAST | BH_SYSCALL_F_SERVICE_CALL;
    assert(bh_syscall_policy_check(&ctx, &desc) == K_ERR_INVALID_ARG);

    // Test FAST + USER_READ (Failure)
    desc.flags = BH_SYSCALL_F_FAST | BH_SYSCALL_F_USER_READ;
    assert(bh_syscall_policy_check(&ctx, &desc) == K_ERR_INVALID_ARG);

    // Test FAST + USER_WRITE (Failure)
    desc.flags = BH_SYSCALL_F_FAST | BH_SYSCALL_F_USER_WRITE;
    assert(bh_syscall_policy_check(&ctx, &desc) == K_ERR_INVALID_ARG);

    // Test FAST (Success)
    desc.flags = BH_SYSCALL_F_FAST;
    assert(bh_syscall_policy_check(&ctx, &desc) == K_OK);

    printf("Policy enforcement tests passed.\n");
}

int main(void) {
    test_policy_enforcement();
    return 0;
}
