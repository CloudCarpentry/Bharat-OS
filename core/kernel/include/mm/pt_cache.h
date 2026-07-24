#ifndef BHARAT_MM_PT_CACHE_H
#define BHARAT_MM_PT_CACHE_H

#include <stdint.h>
#include <stddef.h>
#include "../../include/mm.h"
#include "../../include/slab.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the page-table page cache
void pt_cache_init(void);

// Allocate a single page-table page, zeroed, backed by slab/cache.
phys_addr_t pt_cache_alloc(void);

// Free a single page-table page back to the cache.
void pt_cache_free(phys_addr_t pa);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_MM_PT_CACHE_H
