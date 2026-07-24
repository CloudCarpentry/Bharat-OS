#ifndef BHARAT_MM_USER_RANGE_H
#define BHARAT_MM_USER_RANGE_H

#include <stdint.h>
#include <stddef.h>
#include "kernel/status.h"

#define BH_USER_ACCESS_READ   (1u << 0)
#define BH_USER_ACCESS_WRITE  (1u << 1)
#define BH_USER_ACCESS_EXEC   (1u << 2)

/**
 * @brief Validates a user address range against the current process's address space.
 *
 * This is the Stage 2 usercopy validation adapter.
 *
 * @param ptr Start of the user range.
 * @param len Length of the range.
 * @param access Required access flags (BH_USER_ACCESS_*).
 * @return K_OK if valid, or an appropriate error code.
 */
kstatus_t mm_user_range_validate_current(uintptr_t ptr, size_t len, uint32_t access);

#endif /* BHARAT_MM_USER_RANGE_H */
