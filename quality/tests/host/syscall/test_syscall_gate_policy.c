#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "trap/syscall_context.h"
#include "kernel/status.h"
#include "bharat/personality/personality_interface.h"
#include "profile/profile_policy.h"

// Stubs for functions called by bh_syscall_policy_check
void bh_syscall_stats_inc_denied(uint32_t core_id) { (void)core_id; }
uint32_t hal_cpu_get_id(void) { return 0; }

// Mock profile policy
static bh_profile_policy_t mock_policy = {
    .name = "test-policy",
    .traits = BH_PROFILE_TRAIT_MMU_FULL | BH_PROFILE_TRAIT_SERVICE_RICH,
    .allowed_personalities = (1ULL << BH_PERSONALITY_NATIVE),
    .syscall_policy_flags = 0,
    .max_usercopy_bytes = 4096
};

const bh_profile_policy_t *bh_profile_current_policy(void) {
    return &mock_policy;
}

bool bh_profile_has_trait(uint64_t trait) {
    return (mock_policy.traits & trait) != 0;
}

bool bh_profile_allows_personality(uint32_t personality) {
    return (mock_policy.allowed_personalities & (1ULL << personality)) != 0;
}

bool bh_profile_allows_blocking_syscall(void) {
    if (bh_profile_has_trait(BH_PROFILE_TRAIT_NO_BLOCKING_RT)) return false;
    return true;
}

// Minimal implementation matching core/kernel/src/trap/syscall_gate.c
kstatus_t bh_syscall_policy_check(bh_syscall_ctx_t *ctx, const bh_syscall_desc_t *desc) {
    if (!ctx || !desc) return K_ERR_INVALID_ARG;
    kstatus_t status = K_OK;

    if (!bh_profile_allows_personality(ctx->personality)) return K_ERR_DENIED;

    if (desc->flags & BH_SYSCALL_F_CAP_REQUIRED) {
        if (desc->required_rights == 0) status = K_ERR_DENIED;
    }

    if (status == K_OK && (desc->flags & BH_SYSCALL_F_FAST)) {
        if (desc->flags & (BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_SERVICE_CALL)) status = K_ERR_INVALID_ARG;
        if (status == K_OK && (desc->flags & (BH_SYSCALL_F_USER_READ | BH_SYSCALL_F_USER_WRITE))) status = K_ERR_INVALID_ARG;
    }

    if (status == K_OK) {
        if ((desc->flags & BH_SYSCALL_F_BLOCKING) && !bh_profile_allows_blocking_syscall()) status = K_ERR_DENIED;
        if ((desc->flags & BH_SYSCALL_F_SERVICE_CALL) && !bh_profile_has_trait(BH_PROFILE_TRAIT_SERVICE_RICH)) status = K_ERR_DENIED;
    }

    if (status != K_OK) bh_syscall_stats_inc_denied(hal_cpu_get_id());
    return status;
}

void test_policy_enforcement(void) {
    printf("Testing syscall policy enforcement with traits...\n");
    bh_syscall_ctx_t ctx = { .personality = BH_PERSONALITY_NATIVE };
    bh_syscall_desc_t desc = {0};

    // Test personality allowlist
    ctx.personality = BH_PERSONALITY_LINUX; // Not allowed in mock
    assert(bh_syscall_policy_check(&ctx, &desc) == K_ERR_DENIED);
    ctx.personality = BH_PERSONALITY_NATIVE;

    // Test CAP_REQUIRED without rights (Failure)
    desc.flags = BH_SYSCALL_F_CAP_REQUIRED;
    desc.required_rights = 0;
    assert(bh_syscall_policy_check(&ctx, &desc) == K_ERR_DENIED);

    // Test FAST + BLOCKING (Failure)
    desc.flags = BH_SYSCALL_F_FAST | BH_SYSCALL_F_BLOCKING;
    assert(bh_syscall_policy_check(&ctx, &desc) == K_ERR_INVALID_ARG);

    // Test NO_BLOCKING_RT trait
    mock_policy.traits |= BH_PROFILE_TRAIT_NO_BLOCKING_RT;
    desc.flags = BH_SYSCALL_F_BLOCKING;
    assert(bh_syscall_policy_check(&ctx, &desc) == K_ERR_DENIED);
    mock_policy.traits &= ~BH_PROFILE_TRAIT_NO_BLOCKING_RT;

    // Test SERVICE_RICH trait
    mock_policy.traits &= ~BH_PROFILE_TRAIT_SERVICE_RICH;
    desc.flags = BH_SYSCALL_F_SERVICE_CALL;
    assert(bh_syscall_policy_check(&ctx, &desc) == K_ERR_DENIED);
    mock_policy.traits |= BH_PROFILE_TRAIT_SERVICE_RICH;

    printf("Policy enforcement tests passed.\n");
}

int main(void) {
    test_policy_enforcement();
    return 0;
}
