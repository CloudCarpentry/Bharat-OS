#include "early_alloc.h"

// Defined in linker script
extern uint8_t _end[];

static phys_addr_t early_bump_ptr = 0;

void early_alloc_init(phys_addr_t start_addr) {
    if (start_addr == 0) {
        start_addr = (phys_addr_t)(uintptr_t)_end;
    }
    early_bump_ptr = start_addr;
}

void* early_alloc(size_t size, size_t alignment) {
    if (early_bump_ptr == 0) {
        early_alloc_init(0);
    }

    if (alignment > 0) {
        // Align bump pointer
        phys_addr_t remainder = early_bump_ptr % alignment;
        if (remainder != 0) {
            early_bump_ptr += (alignment - remainder);
        }
    }

    void* ptr = (void*)(uintptr_t)early_bump_ptr;
    early_bump_ptr += size;

    // Zero the allocated memory
    uint8_t* p = (uint8_t*)ptr;
    for (size_t i = 0; i < size; i++) {
        p[i] = 0;
    }

    return ptr;
}

phys_addr_t early_alloc_get_current_ptr(void) {
    if (early_bump_ptr == 0) {
        early_alloc_init(0);
    }
    return early_bump_ptr;
}
