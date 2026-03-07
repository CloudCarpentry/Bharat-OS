#ifndef BHARAT_EARLY_ALLOC_H
#define BHARAT_EARLY_ALLOC_H

#include <stdint.h>
#include <stddef.h>
#include "../../include/mm.h"

/* Initialize the early bump allocator with the start of free memory (usually _end) */
void early_alloc_init(phys_addr_t start_addr);

/* Allocate a zeroed, aligned block of memory */
void* early_alloc(size_t size, size_t alignment);

/* Get the current bump pointer (used to know where available memory for buddy starts) */
phys_addr_t early_alloc_get_current_ptr(void);

#endif // BHARAT_EARLY_ALLOC_H
