#include "init_manifest.h"
#include <stddef.h>
#include <kernel/status.h>

// Mock stubs for entry points in testing environments.
// Real applications will populate these functions natively.
__attribute__((weak)) int dummy_start(void *ctx) { return K_OK; }
__attribute__((weak)) int dummy_probe(void *ctx) { return K_OK; }

static const uint16_t namesvc_deps[] = {};
static const uint16_t pmm_deps[] = {0}; // example, depending on namesvc

// TINY
static const init_service_desc_t manifest_tiny[] = {
    {
        .name = "namesvc",
        .start_fn = dummy_start,
        .probe_fn = dummy_probe,
        .deps = namesvc_deps,
        .dep_count = 0,
        .retry_limit = 1,
        .policy = INIT_SERVICE_OPTIONAL,
        .profile_mask = INIT_PROFILE_MASK_ALL,
        .board_mask = ~0ULL,
        .personality_mask = ~0ULL,
        .required_caps = 0,
    }
};

// SMALL
static const init_service_desc_t manifest_small[] = {
    {
        .name = "namesvc",
        .start_fn = dummy_start,
        .deps = NULL,
        .dep_count = 0,
        .retry_limit = 2,
        .policy = INIT_SERVICE_REQUIRED,
        .profile_mask = INIT_PROFILE_MASK_ALL,
        .board_mask = ~0ULL,
        .personality_mask = ~0ULL,
        .required_caps = 0,
    },
    {
        .name = "devmgr-lite",
        .start_fn = dummy_start,
        .deps = pmm_deps,
        .dep_count = 1,
        .retry_limit = 2,
        .policy = INIT_SERVICE_OPTIONAL,
        .profile_mask = INIT_PROFILE_MASK_ALL,
        .board_mask = ~0ULL,
        .personality_mask = ~0ULL,
        .required_caps = 0,
    }
};

// EMBEDDED_RICH
static const init_service_desc_t manifest_embedded_rich[] = {
    {
        .name = "namesvc",
        .start_fn = dummy_start,
        .deps = NULL,
        .dep_count = 0,
        .retry_limit = 3,
        .policy = INIT_SERVICE_REQUIRED,
        .profile_mask = INIT_PROFILE_MASK_ALL,
        .board_mask = ~0ULL,
        .personality_mask = ~0ULL,
        .required_caps = 0,
    },
    {
        .name = "process_manager",
        .start_fn = dummy_start,
        .deps = pmm_deps,
        .dep_count = 1,
        .retry_limit = 3,
        .policy = INIT_SERVICE_REQUIRED,
        .profile_mask = INIT_PROFILE_MASK_ALL,
        .board_mask = ~0ULL,
        .personality_mask = ~0ULL,
        .required_caps = 0,
    },
    {
        .name = "vm_manager",
        .start_fn = dummy_start,
        .deps = pmm_deps,
        .dep_count = 1,
        .retry_limit = 3,
        .policy = INIT_SERVICE_REQUIRED,
        .profile_mask = INIT_PROFILE_MASK_ALL,
        .board_mask = ~0ULL,
        .personality_mask = ~0ULL,
        .required_caps = 0,
    }
};

// MOBILE
static const init_service_desc_t manifest_mobile[] = {
    {
        .name = "namesvc",
        .start_fn = dummy_start,
        .deps = NULL,
        .dep_count = 0,
        .retry_limit = 3,
        .policy = INIT_SERVICE_REQUIRED,
        .profile_mask = INIT_PROFILE_MASK_ALL,
        .board_mask = ~0ULL,
        .personality_mask = ~0ULL,
        .required_caps = 0,
    },
    {
        .name = "boot_displayd",
        .start_fn = dummy_start,
        .deps = pmm_deps,
        .dep_count = 1,
        .retry_limit = 2,
        .policy = INIT_SERVICE_OPTIONAL,
        .profile_mask = INIT_PROFILE_MASK_ALL,
        .board_mask = ~0ULL,
        .personality_mask = ~0ULL,
        .required_caps = 0,
    }
};

// DESKTOP
static const init_service_desc_t manifest_desktop[] = {
    {
        .name = "namesvc",
        .start_fn = dummy_start,
        .deps = NULL,
        .dep_count = 0,
        .retry_limit = 3,
        .policy = INIT_SERVICE_REQUIRED,
        .profile_mask = INIT_PROFILE_MASK_ALL,
        .board_mask = ~0ULL,
        .personality_mask = ~0ULL,
        .required_caps = 0,
    },
    {
        .name = "boot_displayd",
        .start_fn = dummy_start,
        .deps = pmm_deps,
        .dep_count = 1,
        .retry_limit = 3,
        .policy = INIT_SERVICE_OPTIONAL,
        .profile_mask = INIT_PROFILE_MASK_ALL,
        .board_mask = ~0ULL,
        .personality_mask = ~0ULL,
        .required_caps = 0,
    }
};

// DRONE
static const init_service_desc_t manifest_drone[] = {
    {
        .name = "namesvc",
        .start_fn = dummy_start,
        .deps = NULL,
        .dep_count = 0,
        .retry_limit = 1,
        .policy = INIT_SERVICE_REQUIRED,
        .profile_mask = INIT_PROFILE_MASK_ALL,
        .board_mask = ~0ULL,
        .personality_mask = ~0ULL,
        .required_caps = 0,
    },
    {
        .name = "faultmgr-lite",
        .start_fn = dummy_start,
        .deps = pmm_deps,
        .dep_count = 1,
        .retry_limit = 0,
        .policy = INIT_SERVICE_REQUIRED,
        .profile_mask = INIT_PROFILE_MASK_ALL,
        .board_mask = ~0ULL,
        .personality_mask = ~0ULL,
        .required_caps = 0,
    }
};


int init_manifest_get(bharat_init_profile_t profile, init_manifest_t *out_manifest) {
    if (!out_manifest) {
        return K_ERR_INVALID_ARG;
    }

    switch (profile) {
        case BHARAT_INIT_PROFILE_TINY:
            out_manifest->services = manifest_tiny;
            out_manifest->count = sizeof(manifest_tiny) / sizeof(init_service_desc_t);
            break;
        case BHARAT_INIT_PROFILE_SMALL:
            out_manifest->services = manifest_small;
            out_manifest->count = sizeof(manifest_small) / sizeof(init_service_desc_t);
            break;
        case BHARAT_INIT_PROFILE_EMBEDDED_RICH:
            out_manifest->services = manifest_embedded_rich;
            out_manifest->count = sizeof(manifest_embedded_rich) / sizeof(init_service_desc_t);
            break;
        case BHARAT_INIT_PROFILE_MOBILE:
            out_manifest->services = manifest_mobile;
            out_manifest->count = sizeof(manifest_mobile) / sizeof(init_service_desc_t);
            break;
        case BHARAT_INIT_PROFILE_DESKTOP:
            out_manifest->services = manifest_desktop;
            out_manifest->count = sizeof(manifest_desktop) / sizeof(init_service_desc_t);
            break;
        case BHARAT_INIT_PROFILE_DRONE:
            out_manifest->services = manifest_drone;
            out_manifest->count = sizeof(manifest_drone) / sizeof(init_service_desc_t);
            break;
        default:
            return K_ERR_NOT_FOUND;
    }

    return K_OK;
}
