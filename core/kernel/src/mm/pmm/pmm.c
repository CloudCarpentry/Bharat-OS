/*
 * Physical Memory Manager (PMM) modular facade.
 *
 * Implementation has been modularized into:
 *   - pmm_init.c   (Boot reservations, region mapping, initialization)
 *   - pmm_buddy.c  (Buddy allocation, colored zone allocator, free lists)
 *   - pmm_fault.c  (Physical fault assist & verification helpers)
 */

#include "pmm_internal.h"
