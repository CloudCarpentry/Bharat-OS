#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../kernel/include/hal/hal_pt.h"
#include "../kernel/include/hal/hal_tlb.h"

#define MAX_ENTRIES 128

typedef struct {
    virt_addr_t va;
    phys_addr_t pa;
    uint32_t flags;
    int used;
} ent_t;

static ent_t g_entries[MAX_ENTRIES];
static int g_map_calls = 0;
static int g_unmap_calls = 0;
static int g_protect_calls = 0;
static int g_query_calls = 0;

static int find_slot(virt_addr_t va) {
    for (int i = 0; i < MAX_ENTRIES; ++i) {
        if (g_entries[i].used && g_entries[i].va == va) return i;
    }
    return -1;
}

static int alloc_slot(void) {
    for (int i = 0; i < MAX_ENTRIES; ++i) {
        if (!g_entries[i].used) return i;
    }
    return -1;
}

static phys_addr_t mock_create_as(phys_addr_t kernel_root_table) {
    (void)kernel_root_table;
    return 0x1000;
}

static void mock_destroy_as(phys_addr_t root_pt) {
    (void)root_pt;
}

static int mock_map_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    (void)root_pt;
    ++g_map_calls;
    int idx = find_slot(vaddr);
    if (idx < 0) idx = alloc_slot();
    if (idx < 0) return -1;
    g_entries[idx].used = 1;
    g_entries[idx].va = vaddr;
    g_entries[idx].pa = paddr;
    g_entries[idx].flags = flags;
    return 0;
}

static int mock_unmap_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *unmapped_paddr) {
    (void)root_pt;
    ++g_unmap_calls;
    int idx = find_slot(vaddr);
    if (idx < 0) return -2;
    if (unmapped_paddr) *unmapped_paddr = g_entries[idx].pa;
    g_entries[idx].used = 0;
    return 0;
}

static int mock_protect_page(phys_addr_t root_pt, virt_addr_t vaddr, uint32_t new_flags) {
    (void)root_pt;
    ++g_protect_calls;
    int idx = find_slot(vaddr);
    if (idx < 0) return -2;
    g_entries[idx].flags = new_flags;
    return 0;
}

static int mock_query_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, uint32_t *flags) {
    (void)root_pt;
    ++g_query_calls;
    int idx = find_slot(vaddr);
    if (idx < 0) return -2;
    if (paddr) *paddr = g_entries[idx].pa;
    if (flags) *flags = g_entries[idx].flags;
    return 0;
}

static const hal_pt_caps_t g_pt_caps = {
    .backend_kind = TRANSLATE_BACKEND_MMU,
    .exec_class = TRANSLATE_EXEC_MMU_FULL,
    .supports_sparse_vm = true,
    .supports_query = true,
};

static const hal_tlb_caps_t g_tlb_caps = {
    .supports_all_flush = true,
};

hal_pt_ops_t x86_hal_pt_ops = {
    .backend_type = TRANSLATE_BACKEND_MMU,
    .caps = &g_pt_caps,
    .create_address_space = mock_create_as,
    .destroy_address_space = mock_destroy_as,
    .map_page = mock_map_page,
    .unmap_page = mock_unmap_page,
    .protect_page = mock_protect_page,
    .query_page = mock_query_page,
    .map_range = NULL,
    .unmap_range = NULL,
    .protect_range = NULL,
    .query_mapping = NULL,
};

hal_tlb_ops_t x86_hal_tlb_ops = {
    .caps = &g_tlb_caps,
};

int main(void) {
    phys_addr_t as = hal_pt_create_address_space(0);
    assert(as != 0);

    assert(hal_pt_map_range(as, 0x4000, 0x8000, 3 * PAGE_SIZE, HAL_PT_FLAG_READ | HAL_PT_FLAG_WRITE) == 0);
    assert(g_map_calls == 3);

    assert(hal_pt_protect_range(as, 0x4000, 2 * PAGE_SIZE, HAL_PT_FLAG_READ) == 0);
    assert(g_protect_calls == 2);

    phys_addr_t pa = 0;
    size_t mapped_size = 0;
    uint32_t flags = 0;
    assert(hal_pt_query_mapping(as, 0x4000, &pa, &mapped_size, &flags) == 0);
    assert(pa == 0x8000);
    assert(mapped_size == PAGE_SIZE);
    assert(flags == HAL_PT_FLAG_READ);
    assert(g_query_calls == 1);

    assert(hal_pt_unmap_range(as, 0x4000, 3 * PAGE_SIZE) == 0);
    assert(g_unmap_calls == 3);

    puts("test_hal_pt_fallbacks: PASS");
    return 0;
}
