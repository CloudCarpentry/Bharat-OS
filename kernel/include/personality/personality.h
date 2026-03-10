#ifndef BHARAT_PERSONALITY_H
#define BHARAT_PERSONALITY_H

#include "personality_types.h"

/**
 * @brief OS Personality Framework
 *
 * This layer registers and manages compatibility personalities (e.g. Linux,
 * Windows, Android) allowing them to intercept system calls and provide
 * logical object semantics without muddying the multikernel core.
 */

/**
 * @brief Register a new OS personality descriptor.
 * @param desc The personality descriptor.
 * @return 0 on success, < 0 on error.
 */
int personality_register(personality_desc_t* desc);

/**
 * @brief Lookup a registered personality by its ID.
 * @param id The personality ID.
 * @return Pointer to the descriptor, or NULL if not found.
 */
personality_desc_t* personality_lookup(personality_id_t id);

#endif // BHARAT_PERSONALITY_H
