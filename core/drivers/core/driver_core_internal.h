#ifndef BHARAT_DRIVER_CORE_INTERNAL_H
#define BHARAT_DRIVER_CORE_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Compare two strings for equality.
 * @return true if equal, false otherwise.
 */
static inline bool driver_core_streq(const char *s1, const char *s2) {
    if (!s1 || !s2) return s1 == s2;
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (*(const unsigned char *)s1 - *(const unsigned char *)s2) == 0;
}

/**
 * @brief Check if a string is non-NULL and has non-zero length.
 */
static inline bool driver_core_strlen_nonzero(const char *s) {
    return s && s[0] != '\0';
}

/**
 * @brief Basic validation for device/driver names.
 */
static inline bool driver_core_name_valid(const char *name) {
    return driver_core_strlen_nonzero(name);
}

#endif // BHARAT_DRIVER_CORE_INTERNAL_H
