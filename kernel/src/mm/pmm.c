#include "../../include/mm.h"
#include <stddef.h>

/*
 * Bharat-OS Physical Memory Allocator
 * Uses a basic bitmap to track allocated physical pages (4KB).
 * Future: Buddy allocation system and NUMA-node awareness for Bharat-Cloud.
 */

#define PAGE_SIZE 4096
#define MAX_PHYS_PAGES 1048576 // Supports up to 4GB of physical RAM initially

static uint8_t page_bitmap[MAX_PHYS_PAGES / 8];
static phys_addr_t highest_usable_addr = 0;

// Internal helper to set a bit in the bitmap
static inline void set_page_used(size_t index) {
    page_bitmap[index / 8] |= (1 << (index % 8));
}

// Internal helper to clear a bit in the bitmap
static inline void clear_page_used(size_t index) {
    page_bitmap[index / 8] &= ~(1 << (index % 8));
}

// Internal helper to check a bit
static inline int is_page_used(size_t index) {
    return (page_bitmap[index / 8] & (1 << (index % 8))) != 0;
}

int pmm_init(phys_addr_t start, phys_addr_t end) {
    if (end > MAX_PHYS_PAGES * PAGE_SIZE) {
        highest_usable_addr = MAX_PHYS_PAGES * PAGE_SIZE;
    } else {
        highest_usable_addr = end;
    }

    // Mark everything as used initially to prevent allocating unavailable memory
    for (size_t i = 0; i < (MAX_PHYS_PAGES / 8); i++) {
        page_bitmap[i] = 0xFF;
    }

    // Free the range provided by the bootloader
    size_t start_idx = start / PAGE_SIZE;
    size_t end_idx = highest_usable_addr / PAGE_SIZE;
    
    for (size_t i = start_idx; i < end_idx; i++) {
        clear_page_used(i);
    }
    
    return 0; // Success
}

phys_addr_t pmm_alloc_page() {
    size_t limit = highest_usable_addr / PAGE_SIZE;
    
    for (size_t i = 0; i < limit; i++) {
        if (!is_page_used(i)) {
            set_page_used(i);
            return (phys_addr_t)(i * PAGE_SIZE);
        }
    }
    
    return 0; // Out of memory
}

void pmm_free_page(phys_addr_t addr) {
    size_t index = addr / PAGE_SIZE;
    if (index < (highest_usable_addr / PAGE_SIZE)) {
        clear_page_used(index);
    }
}
