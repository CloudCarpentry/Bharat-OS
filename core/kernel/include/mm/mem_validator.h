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

/**
 * Validates the runtime capabilities against the memory profile contract.
 *
 * @param caps Detected hardware/runtime capabilities.
 * @param contract Requested memory profile contract.
 * @return K_OK on success, K_ERR_PROFILE_RESTRICTED or other error on mismatch.
 */
kstatus_t mem_runtime_validate_profile(const mem_runtime_caps_t *caps,
                                        const mem_profile_contract_t *contract);

/**
 * Converts a validation status code to a human-readable string.
 */
const char* mem_runtime_validation_status_to_string(kstatus_t status);

#endif /* BHARAT_MM_VALIDATOR_H */
