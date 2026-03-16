#ifndef BHARAT_HAL_X86_TRAP_FRAME_H
#define BHARAT_HAL_X86_TRAP_FRAME_H

#include "../../../include/trap.h"

typedef struct {
    trap_frame_t base;    // must be first — generic code casts to this
    uint64_t error_code;  // x86-only: pushed by CPU or trap_entry.S
    uint64_t cr2;         // faulting VA — read once in entry, stored here
} x86_trap_frame_t;

#endif // BHARAT_HAL_X86_TRAP_FRAME_H
