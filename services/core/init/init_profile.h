#ifndef BHARAT_INIT_PROFILE_H
#define BHARAT_INIT_PROFILE_H

#include "init_manifest.h"

typedef struct {
    bharat_init_profile_t profile;
    uint32_t arch_id;
    uint32_t platform_id;
    uint32_t board_id;
    uint32_t personality_id;
    bharat_init_cap_mask_t cap_mask;
    bool safe_mode;
    bool diagnostics_mode;
} init_boot_context_t;

// Initializes the boot context
void init_profile_get_context(init_boot_context_t *ctx);

#endif // BHARAT_INIT_PROFILE_H
