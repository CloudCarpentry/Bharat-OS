#include "../../include/mm_zswap.h"
#include "../../include/atomic.h"
#include "../../include/spinlock.h"
#include "../../include/slab.h"
#include "../../include/list.h"
#include "../../include/profile.h"

#ifdef Profile_RTOS
#undef CONFIG_MM_ZSWAP
#define CONFIG_MM_ZSWAP 0
#endif

typedef struct {
    list_head_t list;
    phys_addr_t original_paddr; // ID of the page
    uint8_t* compressed_data;
    size_t compressed_size;
} zswap_entry_t;

static list_head_t zswap_pool;
static spinlock_t zswap_lock;
static size_t zswap_current_pool_bytes = 0;
static size_t zswap_max_pool_bytes = 0;

static kcache_t* zswap_entry_cache = NULL;

// Very basic LZ4-like compression simulation logic as an actual LZ4 implementation requires complex structures
// Fast run-length encoding (RLE) logic to represent simple compressibility and decompression latency.
int lz4_compress(const uint8_t* src, size_t src_size, uint8_t* dst, size_t max_dst_size) {
    if (!src || !dst || src_size == 0 || max_dst_size == 0) return -1;

    size_t out_idx = 0;
    size_t in_idx = 0;

    while (in_idx < src_size) {
        uint8_t val = src[in_idx];
        uint8_t count = 1;
        while (in_idx + count < src_size && src[in_idx + count] == val && count < 255) {
            count++;
        }

        if (out_idx + 2 > max_dst_size) {
            return -1; // Not enough space
        }

        dst[out_idx++] = count;
        dst[out_idx++] = val;
        in_idx += count;
    }

    return (int)out_idx;
}

int lz4_decompress(const uint8_t* src, size_t src_size, uint8_t* dst, size_t dst_size) {
    if (!src || !dst || src_size == 0 || dst_size == 0) return -1;

    size_t in_idx = 0;
    size_t out_idx = 0;

    while (in_idx < src_size) {
        if (in_idx + 1 >= src_size) return -1; // Corrupted

        uint8_t count = src[in_idx++];
        uint8_t val = src[in_idx++];

        if (out_idx + count > dst_size) return -1; // Buffer overflow

        for (uint8_t i = 0; i < count; i++) {
            dst[out_idx++] = val;
        }
    }

    return (int)out_idx;
}

int zswap_init(void) {
#if CONFIG_MM_ZSWAP == 1
    list_init(&zswap_pool);
    spin_lock_init(&zswap_lock);
    zswap_current_pool_bytes = 0;

    // For simplicity, just use a hardcoded reasonable max (e.g., 64MB)
    // In a real system, this would be derived from total physical memory.
    zswap_max_pool_bytes = 64 * 1024 * 1024;

    zswap_entry_cache = kcache_create("zswap_entry", sizeof(zswap_entry_t));
    if (!zswap_entry_cache) return -1;

    return 0;
#else
    return 0;
#endif
}

int zswap_store_page(phys_addr_t page) {
#if CONFIG_MM_ZSWAP == 1
    if (!page) return -1;

    spin_lock(&zswap_lock);
    if (zswap_current_pool_bytes >= zswap_max_pool_bytes) {
        spin_unlock(&zswap_lock);
        return -1; // Pool full
    }
    spin_unlock(&zswap_lock);

    uint8_t* src = (uint8_t*)(uintptr_t)page; // Assume physical mapping allows this, else map it temporarily
    uint8_t temp_buf[PAGE_SIZE];

    int comp_size = lz4_compress(src, PAGE_SIZE, temp_buf, PAGE_SIZE);

    // Check if compression saved enough space
    if (comp_size < 0 || comp_size > (PAGE_SIZE * CONFIG_MM_COMP_MIN_SAVINGS / 100)) {
        return -1; // Did not compress well enough
    }

    zswap_entry_t* entry = (zswap_entry_t*)kcache_alloc(zswap_entry_cache);
    if (!entry) return -1;

    entry->compressed_data = (uint8_t*)kmalloc(comp_size);
    if (!entry->compressed_data) {
        kcache_free(zswap_entry_cache, entry);
        return -1;
    }

    for (int i = 0; i < comp_size; i++) {
        entry->compressed_data[i] = temp_buf[i];
    }

    entry->original_paddr = page;
    entry->compressed_size = comp_size;

    spin_lock(&zswap_lock);
    list_add(&entry->list, &zswap_pool);
    zswap_current_pool_bytes += comp_size;
    spin_unlock(&zswap_lock);

    return 0; // Success
#else
    (void)page;
    return -1;
#endif
}

int zswap_load_page(phys_addr_t page) {
#if CONFIG_MM_ZSWAP == 1
    if (!page) return -1;

    spin_lock(&zswap_lock);

    list_head_t* pos;
    zswap_entry_t* found_entry = NULL;

    for (pos = zswap_pool.next; pos != &zswap_pool; pos = pos->next) {
        zswap_entry_t* entry = list_entry(pos, zswap_entry_t, list);
        if (entry->original_paddr == page) {
            found_entry = entry;
            list_del(&entry->list);
            zswap_current_pool_bytes -= entry->compressed_size;
            break;
        }
    }

    spin_unlock(&zswap_lock);

    if (!found_entry) return -1; // Not in zswap

    uint8_t* dst = (uint8_t*)(uintptr_t)page; // Assume mapped

    int decomp_size = lz4_decompress(found_entry->compressed_data, found_entry->compressed_size, dst, PAGE_SIZE);

    kfree(found_entry->compressed_data);
    kcache_free(zswap_entry_cache, found_entry);

    if (decomp_size != PAGE_SIZE) return -1; // Decompression failed

    return 0;
#else
    (void)page;
    return -1;
#endif
}
