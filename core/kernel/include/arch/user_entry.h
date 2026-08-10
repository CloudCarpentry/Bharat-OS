#ifndef BHARAT_ARCH_USER_ENTRY_H
#define BHARAT_ARCH_USER_ENTRY_H

#include <stdint.h>
#include "mm/aspace.h"

typedef struct arch_user_entry {
    uintptr_t entry_pc;
    uintptr_t user_sp;
    uintptr_t arg0;
    address_space_t *aspace;
    uint64_t flags;
} arch_user_entry_t;

kstatus_t arch_user_entry_prepare(
    arch_user_entry_t *out,
    address_space_t *aspace,
    uintptr_t entry_pc,
    uintptr_t user_sp,
    uintptr_t arg0);

__attribute__((noreturn))
void arch_enter_user(const arch_user_entry_t *entry);

#endif // BHARAT_ARCH_USER_ENTRY_H
