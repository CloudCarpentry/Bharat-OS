#ifndef BHARAT_HAL_CAPS_H
#define BHARAT_HAL_CAPS_H

/* Internal wrapper around the canonical UAPI capability contract */
#include "../../../uapi/capability/hw_caps.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * HAL-level API to retrieve the current hardware capabilities.
 * Platform-specific discovery code populates the canonical record and
 * exposes it via this interface.
 *
 * @param caps Pointer to structure to be populated with discovered capabilities.
 * @return 0 on success, negative error code on failure.
 */
int hal_get_hw_caps(bharat_hw_caps_t *caps);

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_HAL_CAPS_H */
