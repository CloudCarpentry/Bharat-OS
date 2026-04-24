#ifndef BHARAT_PHYSMAP_H
#define BHARAT_PHYSMAP_H

#include <stdint.h>
#include <stdbool.h>
#include "mm.h"
#include "../hal/hal_pt.h"

/**
 * Direct-Map Linear Mapping Subsystem
 *
 * Replaces raw P2V/V2P macros with a formal API for safely resolving
 * kernel-accessible linear mappings of physical memory.
 */

// Global state initialization for the physmap subsystem
void physmap_init(void);

// Answers: is there a kernel linear physmap?
bool physmap_has_linear_map(void);

// Converts a physical address to a linearly mapped kernel virtual address.
void *physmap_phys_to_virt(phys_addr_t phys);

// Converts a kernel virtual address back to a physical address.
phys_addr_t physmap_virt_to_phys(const void *virt);

// Returns the translation backend type
translate_backend_kind_t physmap_backend_type(void);

// Returns the translation execution class
translate_exec_class_t physmap_exec_class(void);

#endif // BHARAT_PHYSMAP_H
