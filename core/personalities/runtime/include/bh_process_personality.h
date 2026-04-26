#ifndef BH_PROCESS_PERSONALITY_H
#define BH_PROCESS_PERSONALITY_H

#include "bh_personality.h"
#include "bh_error_domain.h"
#include "bh_handle_space.h"
#include <stdint.h>

/**
 * @brief Personality configuration for a process.
 */
typedef struct bh_process_personality {
    bh_personality_kind_t kind;
    bh_error_domain_t error_domain;
    bh_handle_space_kind_t handle_space;
    uint32_t abi_flags;
} bh_process_personality_t;

#endif // BH_PROCESS_PERSONALITY_H
