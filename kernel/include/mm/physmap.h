#ifndef BHARAT_PHYSMAP_H
#define BHARAT_PHYSMAP_H

#include <stdint.h>
#include <stdbool.h>
#include "mm.h"

/**
 * Direct-Map Linear Mapping Subsystem
 *
 * Replaces raw P2V/V2P macros with a formal API for safely resolving
 * kernel-accessible linear mappings of physical memory.
 */

// Global state initialization for the physmap subsystem
void physmap_init(void);

// Converts a physical address to a linearly mapped kernel virtual address.
// Returns NULL if the physical address is not part of the linear map.
void *phys_to_virt_linear(phys_addr_t pa);

// Converts a linearly mapped kernel virtual address back to a physical address.
// Returns 0 if the virtual address is not part of the linear map.
phys_addr_t virt_to_phys_linear(const void *va);

// Checks if the given virtual address is within the kernel's linear map region.
bool virt_is_in_linear_map(const void *va);

// Checks if the given physical address is currently accessible via the linear map.
bool phys_is_linearly_mapped(phys_addr_t pa);

#endif // BHARAT_PHYSMAP_H
