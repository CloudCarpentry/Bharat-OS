#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../../kernel/include/hal/hal_pt.h"
#include "../../kernel/include/hal/hal_tlb.h"

#define MAX_AS 16
#define MAX_MAP 2048

typedef struct {
    phys_addr_t root;
    size_t owned_pt_pages;
    int live;
} mock_as_t;

typedef struct {
    phys_addr_t root;
    virt_addr_t va;
    phys_addr_t pa;
    uint32_t flags;
} map_ent_t;

static mock_as_t g_spaces[MAX_AS];
static map_ent_t g_maps[MAX_MAP];
static size_t g_alloc_pt_pages = 0;
static size_t g_free_pt_pages = 0;
static phys_addr_t g_next_root = 0x1000;

static mock_as_t* find_as(phys_addr_t root) {
    for (size_t i = 0; i < MAX_AS; i++) if (g_spaces[i].live && g_spaces[i].root == root) return &g_spaces[i];
    return NULL;
}

static map_ent_t* find_map(phys_addr_t root, virt_addr_t va) {
    for (size_t i = 0; i < MAX_MAP; i++) if (g_maps[i].root == root && g_maps[i].va == va) return &g_maps[i];
    return NULL;
}

static phys_addr_t mock_create_as(phys_addr_t kernel_root) {
    (void)kernel_root;
    for (size_t i = 0; i < MAX_AS; i++) {
        if (!g_spaces[i].live) {
            g_spaces[i].live = 1;
            g_spaces[i].root = g_next_root;
            g_spaces[i].owned_pt_pages = 1;
            g_next_root += 0x1000;
            g_alloc_pt_pages += 1;
            return g_spaces[i].root;
        }
    }
    return 0;
}

static void mock_destroy_as(phys_addr_t root) {
    mock_as_t* as = find_as(root);
    assert(as);
    g_free_pt_pages += as->owned_pt_pages;
    memset(as, 0, sizeof(*as));
    for (size_t i = 0; i < MAX_MAP; i++) {
        if (g_maps[i].root == root) memset(&g_maps[i], 0, sizeof(g_maps[i]));
    }
}

static int mock_map_range(phys_addr_t root, virt_addr_t va, phys_addr_t pa, size_t size, uint32_t flags) {
    mock_as_t* as = find_as(root);
    if (!as || (size % PAGE_SIZE)) return -1;
    for (size_t off = 0; off < size; off += PAGE_SIZE) {
        map_ent_t* e = find_map(root, va + off);
        if (!e) {
            for (size_t i = 0; i < MAX_MAP; i++) {
                if (g_maps[i].root == 0) { e = &g_maps[i]; break; }
            }
            if (!e) return -2;
            as->owned_pt_pages += 1;
            g_alloc_pt_pages += 1;
        }
        e->root = root;
        e->va = va + off;
        e->pa = pa + off;
        e->flags = flags | HAL_PT_FLAG_READ;
    }
    return 0;
}

static int mock_unmap_range(phys_addr_t root, virt_addr_t va, size_t size) {
    mock_as_t* as = find_as(root);
    if (!as || (size % PAGE_SIZE)) return -1;
    for (size_t off = 0; off < size; off += PAGE_SIZE) {
        map_ent_t* e = find_map(root, va + off);
        if (!e) return -2;
        memset(e, 0, sizeof(*e));
        as->owned_pt_pages -= 1;
        g_free_pt_pages += 1;
    }
    return 0;
}

static int mock_protect_range(phys_addr_t root, virt_addr_t va, size_t size, uint32_t new_flags) {
    if (size % PAGE_SIZE) return -1;
    for (size_t off = 0; off < size; off += PAGE_SIZE) {
        map_ent_t* e = find_map(root, va + off);
        if (!e) return -2;
        e->flags = new_flags | HAL_PT_FLAG_READ;
    }
    return 0;
}

static int mock_query_mapping(phys_addr_t root, virt_addr_t va, phys_addr_t *pa, size_t *mapped_size, uint32_t *flags) {
    map_ent_t* e = find_map(root, va);
    if (!e) return -2;
    if (pa) *pa = e->pa;
    if (mapped_size) *mapped_size = PAGE_SIZE;
    if (flags) *flags = e->flags;
    return 0;
}

static int mock_map_page(phys_addr_t root, virt_addr_t va, phys_addr_t pa, uint32_t flags) {
    return mock_map_range(root, va, pa, PAGE_SIZE, flags);
}

static int mock_unmap_page(phys_addr_t root, virt_addr_t va, phys_addr_t *unmapped_pa) {
    if (unmapped_pa) {
        map_ent_t* e = find_map(root, va);
        *unmapped_pa = e ? e->pa : 0;
    }
    return mock_unmap_range(root, va, PAGE_SIZE);
}

static int mock_protect_page(phys_addr_t root, virt_addr_t va, uint32_t new_flags) {
    return mock_protect_range(root, va, PAGE_SIZE, new_flags);
}

static int mock_query_page(phys_addr_t root, virt_addr_t va, phys_addr_t *pa, uint32_t *flags) {
    return mock_query_mapping(root, va, pa, NULL, flags);
}

static const hal_pt_caps_t g_caps = {
    .backend_kind = TRANSLATE_BACKEND_MMU,
    .exec_class = TRANSLATE_EXEC_MMU_FULL,
    .supports_query = true,
    .supports_range_map = true,
    .supports_range_unmap = true,
    .supports_range_protect = true,
    .supports_linear_physmap = true,
};

static const hal_tlb_caps_t g_tlb_caps = {
    .supports_page_flush = true,
    .supports_all_flush = true,
};

static hal_pt_ops_t g_mock = {
    .caps = &g_caps,
    .create_address_space = mock_create_as,
    .destroy_address_space = mock_destroy_as,
    .map_page = mock_map_page,
    .unmap_page = mock_unmap_page,
    .protect_page = mock_protect_page,
    .query_page = mock_query_page,
    .map_range = mock_map_range,
    .unmap_range = mock_unmap_range,
    .protect_range = mock_protect_range,
    .query_mapping = mock_query_mapping,
};

hal_pt_ops_t x86_hal_pt_ops;
hal_tlb_ops_t x86_hal_tlb_ops;

int main(void) {
    x86_hal_pt_ops = g_mock;
    x86_hal_tlb_ops.caps = &g_tlb_caps;
    assert(hal_pt_caps()->backend_kind == TRANSLATE_BACKEND_MMU);
    assert(hal_tlb_caps()->supports_all_flush);

    for (int i = 0; i < 100; i++) {
        phys_addr_t as = hal_pt_create_address_space(0);
        assert(as != 0);

        assert(hal_pt_map_range(as, 0x400000, 0x800000, 4 * PAGE_SIZE,
            HAL_PT_FLAG_READ | HAL_PT_FLAG_WRITE | HAL_PT_FLAG_USER) == 0);

        phys_addr_t pa = 0;
        uint32_t flags = 0;
        size_t mapped = 0;
        assert(hal_pt_query_mapping(as, 0x400000, &pa, &mapped, &flags) == 0);
        assert(pa == 0x800000);
        assert(mapped == PAGE_SIZE);
        assert(flags & HAL_PT_FLAG_USER);

        assert(hal_pt_protect_range(as, 0x400000, PAGE_SIZE, HAL_PT_FLAG_READ) == 0);
        assert(hal_pt_query_mapping(as, 0x400000, NULL, NULL, &flags) == 0);
        assert((flags & HAL_PT_FLAG_WRITE) == 0);

        assert(hal_pt_unmap_range(as, 0x400000, 4 * PAGE_SIZE) == 0);
        hal_pt_destroy_address_space(as);
    }

    // Verify generic page-by-page fallbacks when range ops are unavailable.
    x86_hal_pt_ops.map_range = NULL;
    x86_hal_pt_ops.unmap_range = NULL;
    x86_hal_pt_ops.protect_range = NULL;
    x86_hal_pt_ops.query_mapping = NULL;

    phys_addr_t as = hal_pt_create_address_space(0);
    assert(as != 0);
    assert(hal_pt_map_range(as, 0x900000, 0xA00000, 2 * PAGE_SIZE, HAL_PT_FLAG_READ | HAL_PT_FLAG_WRITE) == 0);
    assert(hal_pt_protect_range(as, 0x900000, PAGE_SIZE, HAL_PT_FLAG_READ) == 0);
    assert(hal_pt_query_mapping(as, 0x900000, NULL, NULL, NULL) == 0);
    assert(hal_pt_unmap_range(as, 0x900000, 2 * PAGE_SIZE) == 0);
    hal_pt_destroy_address_space(as);

    assert(g_alloc_pt_pages == g_free_pt_pages);
    printf("pt_common_tests: PASS\n");
    return 0;
}
int tlb_init(void) { return 0; }
