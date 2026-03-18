#ifndef BHARAT_SECURE_MEM_H
#define BHARAT_SECURE_MEM_H

#include <stdint.h>
#include <stddef.h>
#include "mm.h"

/* Secure Buffer Flags */
#define SECURE_MEM_ZERO_ON_FREE  (1U << 0)
#define SECURE_MEM_NODUMP        (1U << 1)
#define SECURE_MEM_NOSHARE       (1U << 2)
#define SECURE_MEM_DMA_PINNED    (1U << 3)

/* Explicit memory zeroing that won't be optimized away */
void secure_memzero_explicit(void *s, size_t n);

/* Secure page allocation */
void *secure_page_alloc(uint32_t flags);

/* Secure page free */
void secure_page_free(void *page, uint32_t flags);

/* Map a secure buffer for a service */
int secure_buffer_map_for_service(uint64_t vaddr, uint64_t size, uint32_t flags);

/* Unmap a secure buffer for a service */
int secure_buffer_unmap_for_service(uint64_t vaddr, uint64_t size);

#endif /* BHARAT_SECURE_MEM_H */
