#ifndef BHARAT_MM_VALIDATOR_H
#define BHARAT_MM_VALIDATOR_H

#include "mm/mem_model.h"
#include "hal/hal_mmu.h"

/**
 * Validates the hardware memory capabilities against the requested OS profile.
 *
 * @return 0 on success, negative error code on failure.
 */
int mm_validate_model(void);

/**
 * Returns the validated memory model currently in use.
 */
mem_model_t mm_get_validated_model(void);

#endif /* BHARAT_MM_VALIDATOR_H */
