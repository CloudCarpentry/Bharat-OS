#ifndef BHARAT_MM_ZSWAP_H
#define BHARAT_MM_ZSWAP_H

#include <stdint.h>
#include <stddef.h>
#include "mm.h"

// Config Options
#ifndef CONFIG_MM_ZSWAP
#define CONFIG_MM_ZSWAP 1
#endif

#ifndef CONFIG_MM_COMP_LZ4
#define CONFIG_MM_COMP_LZ4 1
#endif

#ifndef CONFIG_MM_COMP_POOL_LIMIT_PERCENT
#define CONFIG_MM_COMP_POOL_LIMIT_PERCENT 20
#endif

#ifndef CONFIG_MM_COMP_MIN_SAVINGS
#define CONFIG_MM_COMP_MIN_SAVINGS 80 // percentage of original size
#endif

// Fast LZ4 compression/decompression stubs
int lz4_compress(const uint8_t* src, size_t src_size, uint8_t* dst, size_t max_dst_size);
int lz4_decompress(const uint8_t* src, size_t src_size, uint8_t* dst, size_t dst_size);

int zswap_init(void);

// Reclaim candidate
int zswap_store_page(phys_addr_t page);

// Fault handler integration
int zswap_load_page(phys_addr_t page);

#endif // BHARAT_MM_ZSWAP_H
