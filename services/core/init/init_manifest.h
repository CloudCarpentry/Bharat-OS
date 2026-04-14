#ifndef BHARAT_INIT_MANIFEST_H
#define BHARAT_INIT_MANIFEST_H

#include "init_profile.h"
#include <stdint.h>

typedef enum {
    INIT_SERVICE_DISABLED = 0,
    INIT_SERVICE_OPTIONAL,
    INIT_SERVICE_REQUIRED,
} init_service_policy_t;

typedef struct {
    const char *name;
    int (*start_fn)(void *ctx);
    int (*probe_fn)(void *ctx);      // optional capability/platform precheck
    const uint16_t *deps;
    uint8_t dep_count;
    uint8_t retry_limit;
    init_service_policy_t policy;
    bharat_init_profile_mask_t profile_mask;
    bharat_init_board_mask_t board_mask;
    bharat_init_personality_mask_t personality_mask;
    bharat_init_cap_mask_t required_caps;
} init_service_desc_t;

typedef struct {
    const init_service_desc_t *services;
    uint32_t count;
} init_manifest_t;

/* Get the static manifest for the given profile */
int init_manifest_get(bharat_init_profile_t profile, init_manifest_t *out_manifest);

#endif /* BHARAT_INIT_MANIFEST_H */
