#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kernel/status.h"
#include "mm/hmem.h"
#include "mm/pmm.h"
#include "mm/physmap.h"
#include "slab.h"
#include "spinlock.h"

/* --- Mock Implementations --- */

static uint32_t g_mock_cpu_id = 0;
uint32_t hal_cpu_get_id(void) {
    return g_mock_cpu_id;
}

static uint8_t g_last_freed_buffer[4096];
static size_t g_last_freed_size = 0;
static bool g_track_freed_content = false;

int pmm_alloc_contiguous_ex(uint32_t page_count,
                            pmm_zone_t zone,
                            alloc_class_t cls,
                            uint32_t alloc_flags,
                            pmm_block_t *out_block) {
    (void)zone;
    (void)cls;
    if (!out_block) return -1;

    size_t sz = (size_t)page_count * PAGE_SIZE;
    void *ptr = malloc(sz);
    if (!ptr) return -1;

    if (alloc_flags & PMM_ALLOC_ZERO) {
        memset(ptr, 0, sz);
    } else {
        /* poison with non-zero so we can test ZERO_ON_ALLOC */
        memset(ptr, 0x5A, sz);
    }

    out_block->phys_addr = (uint64_t)(uintptr_t)ptr;
    out_block->page_count = page_count;
    out_block->order = 0;
    out_block->flags = alloc_flags;
    out_block->alloc_class = cls;
    return 0;
}

int pmm_free_pages(const pmm_block_t *block) {
    if (!block || !block->phys_addr) return -1;
    void *ptr = (void *)(uintptr_t)block->phys_addr;
    if (g_track_freed_content && block->page_count > 0) {
        size_t copy_sz = block->page_count * PAGE_SIZE;
        if (copy_sz > sizeof(g_last_freed_buffer)) {
            copy_sz = sizeof(g_last_freed_buffer);
        }
        memcpy(g_last_freed_buffer, ptr, copy_sz);
        g_last_freed_size = copy_sz;
    }
    free(ptr);
    return 0;
}

void *physmap_phys_to_virt(phys_addr_t paddr) {
    return (void *)(uintptr_t)paddr;
}

vm_object_t *vm_object_create_dma(phys_addr_t paddr, size_t size, uint32_t dma_flags, uint32_t numa_node, uint32_t flags) {
    (void)paddr;
    (void)size;
    (void)dma_flags;
    (void)numa_node;
    (void)flags;
    return (vm_object_t *)0xDEADBEEF;
}

void vm_object_release(vm_object_t *obj) {
    (void)obj;
}

void secure_memzero(void *ptr, size_t size) {
    volatile uint8_t *p = (volatile uint8_t *)ptr;
    while (size--) {
        *p++ = 0;
    }
}

void arch_cpu_relax(void) {
#if defined(__x86_64__) || defined(_M_X64)
    __asm__ volatile("pause" ::: "memory");
#else
    __asm__ volatile("" ::: "memory");
#endif
}

hal_hmem_status_t hal_hmem_map_device(const hal_hmem_device_map_req_t *req,
                                      hal_hmem_device_map_result_t *out) {
    (void)req;
    if (out) out->device_address = 0x10000000ULL;
    return HAL_HMEM_OK;
}

hal_hmem_status_t hal_hmem_unmap_device(const hal_hmem_device_unmap_req_t *req) {
    (void)req;
    return HAL_HMEM_OK;
}

hal_hmem_status_t hal_hmem_sync_range(uintptr_t addr, size_t length, hal_hmem_sync_direction_t direction) {
    (void)addr;
    (void)length;
    (void)direction;
    return HAL_HMEM_OK;
}

void *kmalloc(size_t size) {
    return malloc(size);
}

void kfree(void *ptr) {
    free(ptr);
}

void *kmem_aligned_alloc(size_t alignment, size_t size) {
    (void)alignment;
    return malloc(size);
}

void kmem_aligned_free(void *ptr) {
    free(ptr);
}

size_t string_length(const char *str) {
    return strlen(str);
}

/* --- Tests --- */

static void test_hmem_create_invalid_args(void) {
    bharat_hmem_handle_t handle = 0;
    bharat_hmem_desc_v1_t desc;
    memset(&desc, 0, sizeof(desc));

    /* Null desc */
    assert(bh_hmem_create(NULL, BH_HMEM_RIGHT_READ, &handle) == K_ERR_INVALID_ARG);

    /* Null handle out */
    desc.version = BH_HMEM_DESC_VERSION_1;
    desc.struct_size = sizeof(desc);
    desc.size = 4096;
    desc.alignment = 64;
    desc.preferred_domain = BH_HMEM_DOMAIN_SYSTEM;
    assert(bh_hmem_create(&desc, BH_HMEM_RIGHT_READ, NULL) == K_ERR_INVALID_ARG);

    /* Zero size */
    desc.size = 0;
    assert(bh_hmem_create(&desc, BH_HMEM_RIGHT_READ, &handle) == K_ERR_INVALID_ARG);

    /* Non-power-of-two alignment */
    desc.size = 4096;
    desc.alignment = 63;
    assert(bh_hmem_create(&desc, BH_HMEM_RIGHT_READ, &handle) == K_ERR_ALIGNMENT);

    /* Unsupported domain */
    desc.alignment = 64;
    desc.preferred_domain = 999;
    assert(bh_hmem_create(&desc, BH_HMEM_RIGHT_READ, &handle) == K_ERR_INVALID_ARG);

    printf("PASS: test_hmem_create_invalid_args\n");
}

static void test_hmem_lifecycle_basic(void) {
    bharat_hmem_desc_v1_t desc;
    memset(&desc, 0, sizeof(desc));
    desc.version = BH_HMEM_DESC_VERSION_1;
    desc.struct_size = sizeof(desc);
    desc.size = 8192;
    desc.alignment = 64;
    desc.usage_flags = BH_HMEM_USAGE_CPU_READ | BH_HMEM_USAGE_CPU_WRITE;
    desc.property_flags = BH_HMEM_PROP_CPU_COHERENT | BH_HMEM_PROP_ZERO_ON_ALLOC;
    desc.preferred_domain = BH_HMEM_DOMAIN_SYSTEM;

    uint64_t rights = BH_HMEM_RIGHT_READ | BH_HMEM_RIGHT_WRITE | BH_HMEM_RIGHT_MAP_CPU;
    bharat_hmem_handle_t handle = 0;

    assert(bh_hmem_create(&desc, rights, &handle) == K_OK);
    assert(handle != 0);

    /* Query info */
    bharat_hmem_info_v1_t info;
    assert(bh_hmem_get_info(handle, &info) == K_OK);
    assert(info.size == 8192);
    assert(info.alignment == 64);
    assert(info.usage_flags == (BH_HMEM_USAGE_CPU_READ | BH_HMEM_USAGE_CPU_WRITE));
    assert(info.content_generation == 1);

    /* Map CPU */
    uintptr_t va = 0;
    assert(bh_hmem_map_cpu(handle, BH_HMEM_ACCESS_READ | BH_HMEM_ACCESS_WRITE, &va) == K_OK);
    assert(va != 0);

    /* Verify memory was zeroed on alloc */
    uint8_t *bytes = (uint8_t *)va;
    for (size_t i = 0; i < 8192; ++i) {
        assert(bytes[i] == 0);
    }

    /* Write some data */
    memset(bytes, 0x42, 8192);

    /* Re-map while mapped must fail with K_ERR_BUSY */
    uintptr_t va2 = 0;
    assert(bh_hmem_map_cpu(handle, BH_HMEM_ACCESS_READ, &va2) == K_ERR_BUSY);

    /* W^X Violation: Attempt write + execute must fail closed with K_ERR_DENIED */
    assert(bh_hmem_map_cpu(handle, BH_HMEM_ACCESS_WRITE | BH_HMEM_ACCESS_EXECUTE, &va2) == K_ERR_DENIED);

    /* Unmap CPU */
    assert(bh_hmem_unmap_cpu(handle) == K_OK);

    /* Verify content generation bumped on unmap */
    assert(bh_hmem_get_info(handle, &info) == K_OK);
    assert(info.content_generation == 2);

    /* Re-unmap while unmapped must fail with K_ERR_BAD_STATE */
    assert(bh_hmem_unmap_cpu(handle) == K_ERR_BAD_STATE);

    /* Destroy */
    assert(bh_hmem_destroy(handle) == K_OK);

    /* Double destroy must fail with K_ERR_NOT_FOUND */
    assert(bh_hmem_destroy(handle) == K_ERR_NOT_FOUND);

    /* Lookup on destroyed handle must fail with K_ERR_NOT_FOUND */
    assert(bh_hmem_get_info(handle, &info) == K_ERR_NOT_FOUND);

    printf("PASS: test_hmem_lifecycle_basic\n");
}

static void test_hmem_zero_on_free(void) {
    bharat_hmem_desc_v1_t desc;
    memset(&desc, 0, sizeof(desc));
    desc.version = BH_HMEM_DESC_VERSION_1;
    desc.struct_size = sizeof(desc);
    desc.size = 4096;
    desc.alignment = 64;
    desc.usage_flags = BH_HMEM_USAGE_CPU_READ | BH_HMEM_USAGE_CPU_WRITE;
    desc.property_flags = BH_HMEM_PROP_ZERO_ON_FREE;
    desc.preferred_domain = BH_HMEM_DOMAIN_SYSTEM;

    uint64_t rights = BH_HMEM_RIGHT_READ | BH_HMEM_RIGHT_WRITE | BH_HMEM_RIGHT_MAP_CPU;
    bharat_hmem_handle_t handle = 0;

    assert(bh_hmem_create(&desc, rights, &handle) == K_OK);

    uintptr_t va = 0;
    assert(bh_hmem_map_cpu(handle, BH_HMEM_ACCESS_READ | BH_HMEM_ACCESS_WRITE, &va) == K_OK);
    memset((void *)va, 0xEE, 4096);
    assert(bh_hmem_unmap_cpu(handle) == K_OK);

    /* Enable tracking to observe what was freed */
    g_track_freed_content = true;
    memset(g_last_freed_buffer, 0xFF, sizeof(g_last_freed_buffer));
    g_last_freed_size = 0;

    assert(bh_hmem_destroy(handle) == K_OK);

    assert(g_last_freed_size == 4096);
    for (size_t i = 0; i < 4096; ++i) {
        assert(g_last_freed_buffer[i] == 0);
    }
    g_track_freed_content = false;

    printf("PASS: test_hmem_zero_on_free\n");
}

static void test_hmem_stale_handle_rejection(void) {
    bharat_hmem_desc_v1_t desc;
    memset(&desc, 0, sizeof(desc));
    desc.version = BH_HMEM_DESC_VERSION_1;
    desc.struct_size = sizeof(desc);
    desc.size = 4096;
    desc.alignment = 64;
    desc.usage_flags = BH_HMEM_USAGE_CPU_READ | BH_HMEM_USAGE_CPU_WRITE;
    desc.property_flags = BH_HMEM_PROP_CPU_COHERENT;
    desc.preferred_domain = BH_HMEM_DOMAIN_SYSTEM;

    uint64_t rights = BH_HMEM_RIGHT_READ | BH_HMEM_RIGHT_WRITE;
    bharat_hmem_handle_t h1 = 0, h2 = 0;

    assert(bh_hmem_create(&desc, rights, &h1) == K_OK);
    assert(bh_hmem_destroy(h1) == K_OK);

    /* Create new object on same core, should reuse the slot with incremented generation */
    assert(bh_hmem_create(&desc, rights, &h2) == K_OK);
    assert(h1 != h2);

    /* Old handle must be rejected with K_ERR_CAP_STALE (stale generation) */
    bharat_hmem_info_v1_t info;
    assert(bh_hmem_get_info(h1, &info) == K_ERR_CAP_STALE);
    assert(bh_hmem_destroy(h1) == K_ERR_CAP_STALE);

    /* New handle works */
    assert(bh_hmem_get_info(h2, &info) == K_OK);
    assert(bh_hmem_destroy(h2) == K_OK);

    printf("PASS: test_hmem_stale_handle_rejection\n");
}

int main(void) {
    printf("--- Running HMEM Lifecycle & Security Tests ---\n");
    test_hmem_create_invalid_args();
    test_hmem_lifecycle_basic();
    test_hmem_zero_on_free();
    test_hmem_stale_handle_rejection();
    printf("=== All HMEM Lifecycle Tests PASSED ===\n");
    return 0;
}
