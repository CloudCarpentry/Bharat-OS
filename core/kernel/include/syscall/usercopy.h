#ifndef BHARAT_SYSCALL_USERCOPY_H
#define BHARAT_SYSCALL_USERCOPY_H

#include <stdint.h>
#include <stddef.h>
#include <bharat/uapi/syscall/bh_syscall_status.h>

/**
 * Production Centralized Usercopy API for Syscalls.
 *
 * These functions wrap low-level trap-safe copy logic and perform
 * necessary range and permission validation before copying.
 */

bh_status_t bh_copy_from_user(void *dst, const void *src_user, size_t len);
bh_status_t bh_copy_to_user(void *dst_user, const void *src, size_t len);

typedef enum bh_user_access {
    BH_USER_ACCESS_READ  = (1u << 0),
    BH_USER_ACCESS_WRITE = (1u << 1),
} bh_user_access_t;

bh_status_t bh_user_range_validate(const void *ptr, size_t len, bh_user_access_t access);

#endif /* BHARAT_SYSCALL_USERCOPY_H */
