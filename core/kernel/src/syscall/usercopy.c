#include "syscall/usercopy.h"
#include "trap/usercopy.h"
#include "trap/syscall_status.h"
#include "mm/user_range.h"

bh_status_t bh_copy_from_user(void *dst, const void *src_user, size_t len) {
    kstatus_t st = copy_from_user_checked(dst, (uintptr_t)src_user, len);
    return kstatus_to_bh_status(st);
}

bh_status_t bh_copy_to_user(void *dst_user, const void *src, size_t len) {
    kstatus_t st = copy_to_user_checked((uintptr_t)dst_user, src, len);
    return kstatus_to_bh_status(st);
}

bh_status_t bh_user_range_validate(const void *ptr, size_t len, bh_user_access_t access) {
    // Current mm_user_range_validate_current uses BH_USER_ACCESS_READ/WRITE bitmask.
    // Ensure our bh_user_access_t matches or translates it correctly.
    kstatus_t st = mm_user_range_validate_current((uintptr_t)ptr, len, (uint32_t)access);
    return kstatus_to_bh_status(st);
}
