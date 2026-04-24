#ifndef BHARAT_HW_CAPS_REGISTRY_H
#define BHARAT_HW_CAPS_REGISTRY_H

#include "bharat/hw_caps.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Register the boot-discovered hardware capabilities with the kernel registry.
 * This is an immutable operation; once registered, the capabilities cannot be
 * altered to enforce the rule: "no component is allowed to overwrite resolved
 * virtual addresses with raw physical discovery data."
 *
 * @param caps The discovered capability record.
 * @return 0 on success, or a negative error code.
 */
int hw_caps_registry_init(const bharat_hw_caps_t *caps);

/**
 * Retrieve the global capability record.
 *
 * @param out_caps Pointer to populate with the registered capabilities.
 * @return 0 on success, or a negative error code if not initialized.
 */
int hw_caps_registry_get_global(bharat_hw_caps_t *out_caps);

/**
 * Run internal selftest to verify capability registration and retrieval.
 * Must be hooked into the existing kernel selftest framework.
 */
int hw_caps_registry_selftest(void);

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_HW_CAPS_REGISTRY_H */
