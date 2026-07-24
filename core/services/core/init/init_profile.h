#ifndef BHARAT_INIT_PROFILE_H
#define BHARAT_INIT_PROFILE_H

#include <bharat/uapi/init/init_boot_context.h>
#include <stdbool.h>

typedef struct {
    const char *name;
    bool strict_core_deadlines;
    bool quiesce_after_handoff;
    bool allow_optional_failure;
} init_profile_policy_t;

void init_profile_get_context(init_boot_context_t *ctx);
const init_profile_policy_t *init_profile_get_policy(init_profile_t profile);

// Profile conversion helpers
uint64_t init_profile_to_mask(init_profile_t profile);
bool init_profile_mask_allows(uint64_t mask, init_profile_t profile);
const char *init_profile_name(init_profile_t profile);

#endif // BHARAT_INIT_PROFILE_H
