#ifndef BHARAT_MM_ASPACE_PROFILE_H
#define BHARAT_MM_ASPACE_PROFILE_H

#include <stdbool.h>
#include "mem_model.h"

/**
 * Address Space Profiles
 *
 * Defines the runtime protection-context styles that sit on top of the canonical
 * hardware memory models (mem_model_t). These profiles dictate the "shape"
 * and expected semantics of an address space on the running target.
 */
typedef enum {
    ASPACE_PROFILE_NONE = 0,

    /**
     * Rich per-process address spaces with full page-granular protection
     * and dynamic virtual memory semantics.
     * Typical for MEM_MODEL_MMU_FULL.
     */
    ASPACE_PROFILE_FULL,

    /**
     * Shared kernel / constrained user split model.
     * Typical for MEM_MODEL_MMU_LITE or MEM_MODEL_MMU_FULL in constrained modes.
     */
    ASPACE_PROFILE_SPLIT,

    /**
     * Minimal protected flat mapping model.
     * Often used during bring-up, simple execution models, or transitional contexts.
     */
    ASPACE_PROFILE_FLAT,

    /**
     * Region-only protection context (e.g. MPU).
     * No fake sparse paging assumptions or deep VM orchestration.
     * Mandatory for MEM_MODEL_MPU.
     */
    ASPACE_PROFILE_REGION_ONLY
} aspace_profile_t;

/**
 * Maps a canonical memory model to its primary supported address space profile.
 */
static inline aspace_profile_t aspace_profile_get_for_model(mem_model_t model) {
    switch (model) {
        case MEM_MODEL_MMU_FULL:
            return ASPACE_PROFILE_FULL; // Though SPLIT may also be supported
        case MEM_MODEL_MMU_LITE:
            return ASPACE_PROFILE_SPLIT; // Or FLAT
        case MEM_MODEL_MPU:
            return ASPACE_PROFILE_REGION_ONLY;
        default:
            return ASPACE_PROFILE_NONE;
    }
}

/**
 * Gets the active address space profile for the current system configuration.
 */
static inline aspace_profile_t aspace_profile_get_current(void) {
    return aspace_profile_get_for_model(mem_model_get_current());
}

/**
 * Checks if a specific address space profile is legally supported by the given hardware memory model.
 */
static inline bool aspace_profile_is_supported(mem_model_t model, aspace_profile_t profile) {
    switch (model) {
        case MEM_MODEL_MMU_FULL:
            return (profile == ASPACE_PROFILE_FULL || profile == ASPACE_PROFILE_SPLIT);
        case MEM_MODEL_MMU_LITE:
            return (profile == ASPACE_PROFILE_SPLIT || profile == ASPACE_PROFILE_FLAT);
        case MEM_MODEL_MPU:
            return (profile == ASPACE_PROFILE_REGION_ONLY);
        default:
            return false;
    }
}

/**
 * Returns a human-readable name for the address space profile.
 */
static inline const char *aspace_profile_name(aspace_profile_t profile) {
    switch (profile) {
        case ASPACE_PROFILE_FULL:        return "FULL";
        case ASPACE_PROFILE_SPLIT:       return "SPLIT";
        case ASPACE_PROFILE_FLAT:        return "FLAT";
        case ASPACE_PROFILE_REGION_ONLY: return "REGION_ONLY";
        default:                         return "NONE";
    }
}

#endif // BHARAT_MM_ASPACE_PROFILE_H
