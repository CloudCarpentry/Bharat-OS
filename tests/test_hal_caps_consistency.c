#include <assert.h>
#include <stdio.h>

#include "../kernel/include/hal/hal_pt.h"
#include "../kernel/include/hal/hal_tlb.h"

static const hal_pt_caps_t g_pt_caps = {
    .backend_kind = TRANSLATE_BACKEND_MMU,
    .exec_class = TRANSLATE_EXEC_MMU_FULL,
    .supports_sparse_vm = true,
};

static const hal_tlb_caps_t g_tlb_caps = {
    .supports_page_flush = true,
    .supports_all_flush = true,
};

static phys_addr_t mock_create_as(phys_addr_t kernel_root_table) {
    (void)kernel_root_table;
    return 0x2000;
}

static void mock_destroy_as(phys_addr_t root_pt) {
    (void)root_pt;
}

hal_pt_ops_t x86_hal_pt_ops = {
    .backend_type = TRANSLATE_BACKEND_MMU,
    .caps = &g_pt_caps,
    .create_address_space = mock_create_as,
    .destroy_address_space = mock_destroy_as,
};

hal_tlb_ops_t x86_hal_tlb_ops = {
    .caps = &g_tlb_caps,
};

int main(void) {
    hal_pt_init();

    const hal_pt_caps_t *pt_caps = hal_pt_caps();
    const hal_tlb_caps_t *tlb_caps = hal_tlb_caps();

    assert(active_hal_pt != NULL);
    assert(active_hal_tlb != NULL);
    assert(pt_caps != NULL);
    assert(tlb_caps != NULL);
    assert(active_hal_pt->caps == pt_caps);
    assert(active_hal_tlb->caps == tlb_caps);
    assert(pt_caps->backend_kind == TRANSLATE_BACKEND_MMU);
    assert(tlb_caps->supports_all_flush);

    puts("test_hal_caps_consistency: PASS");
    return 0;
}
