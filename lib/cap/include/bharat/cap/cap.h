#ifndef BHARAT_CAP_H
#define BHARAT_CAP_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file cap.h
 * @brief Bharat-OS user-space capability wrapper.
 */

/* Capability handle type */
typedef uint64_t bharat_cap_handle_t;

/* Invalid handle identifier */
#define BHARAT_CAP_INVALID_HANDLE ((bharat_cap_handle_t)0)

/* Capability rights bitmask type */
typedef uint32_t bharat_cap_rights_t;

/* Basic rights examples (subject to expansion) */
#define BHARAT_CAP_RIGHT_READ    (1U << 0)
#define BHARAT_CAP_RIGHT_WRITE   (1U << 1)
#define BHARAT_CAP_RIGHT_EXECUTE (1U << 2)
#define BHARAT_CAP_RIGHT_GRANT   (1U << 3)

/**
 * @brief Validates if a capability handle is non-zero.
 * @param handle The capability handle to check.
 * @return true if handle is valid, false otherwise.
 */
bool bharat_cap_is_valid(bharat_cap_handle_t handle);

/**
 * @brief Formats a capability handle into a string for debugging.
 * @param handle The capability handle.
 * @param buf Output buffer.
 * @param len Buffer length.
 */
void bharat_cap_format(bharat_cap_handle_t handle, char *buf, uint32_t len);

/**
 * @brief Intersects two rights masks.
 * @param a First rights mask.
 * @param b Second rights mask.
 * @return The intersected rights mask.
 */
bharat_cap_rights_t bharat_cap_intersect_rights(bharat_cap_rights_t a, bharat_cap_rights_t b);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_CAP_H
