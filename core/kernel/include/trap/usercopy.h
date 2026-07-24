#ifndef BHARAT_USERCOPY_H
#define BHARAT_USERCOPY_H

#include <stdint.h>
#include <stddef.h>
#include "kernel/status.h"

kstatus_t copy_from_user_checked(void *dst, uintptr_t src, size_t len);
kstatus_t copy_to_user_checked(uintptr_t dst, const void *src, size_t len);
kstatus_t copy_user_string_checked(char *dst, uintptr_t src, size_t max_len);

#endif /* BHARAT_USERCOPY_H */
