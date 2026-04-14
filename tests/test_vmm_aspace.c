#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../kernel/include/mm/aspace.h"
#include "../kernel/include/mm/vm_object.h"
#include "../kernel/include/hal/hal_pt.h"
#include "../kernel/include/mm/aspace_profile.h"

#define TEST(name) void name()
#define ASSERT_EQ(a, b) assert((a) == (b))
#define ASSERT_NE(a, b) assert((a) != (b))
#define ASSERT_NOT_NULL(a) assert((a) != NULL)
#define VM_PROT_READ 1
#define VM_PROT_WRITE 2

extern void hal_pt_init(void);

// Mocks for tests
hal_pt_ops_t mock_pt_ops = {0};

phys_addr_t mock_create_address_space(phys_addr_t root) {
    return root;
}

void mock_destroy_address_space(phys_addr_t root) {
    (void)root;
}

// In benchmark_stubs.c there's probably active_hal_pt.
// We just assign to it.
void hal_pt_init(void) {
    mock_pt_ops.create_address_space = mock_create_address_space;
    mock_pt_ops.destroy_address_space = mock_destroy_address_space;
    active_hal_pt = &mock_pt_ops;
}

phys_addr_t vmm_get_kernel_root(void) {
    return 0x1000;
}

TEST(aspace_overlap_rejected) {
    printf("Running aspace_overlap_rejected...\n");
    address_space_t *as = NULL;
    ASSERT_EQ(0, aspace_create(&as, 0));

    vm_object_t *obj = vm_object_create_anon(0x4000, 0);
    ASSERT_NOT_NULL(obj);

    ASSERT_EQ(0, aspace_region_attach(as, 0x100000, 0x2000, VM_PROT_READ, 0,
                                      VM_INHERIT_COPY_META, obj, 0, NULL));

    ASSERT_NE(0, aspace_region_attach(as, 0x101000, 0x2000, VM_PROT_READ, 0,
                                      VM_INHERIT_COPY_META, obj, 0, NULL));

    aspace_destroy(as);
    vm_object_release(obj);
    printf("Passed aspace_overlap_rejected\n");
}

TEST(aspace_shared_object_two_spaces) {
    printf("Running aspace_shared_object_two_spaces...\n");
    address_space_t *a = NULL, *b = NULL;
    ASSERT_EQ(0, aspace_create(&a, 0));
    ASSERT_EQ(0, aspace_create(&b, 0));

    vm_object_t *obj = vm_object_create_shared(0x4000, 0);
    ASSERT_NOT_NULL(obj);

    uint32_t start_ref = obj->refcount;

    ASSERT_EQ(0, aspace_region_attach(a, 0x200000, 0x2000, VM_PROT_READ|VM_PROT_WRITE, 0,
                                      VM_INHERIT_SHARE, obj, 0, NULL));
    ASSERT_EQ(0, aspace_region_attach(b, 0x300000, 0x2000, VM_PROT_READ|VM_PROT_WRITE, 0,
                                      VM_INHERIT_SHARE, obj, 0, NULL));

    vm_region_t *ra = aspace_lookup_region(a, 0x200100);
    vm_region_t *rb = aspace_lookup_region(b, 0x300100);

    ASSERT_NOT_NULL(ra);
    ASSERT_NOT_NULL(rb);
    ASSERT_EQ(ra->object, rb->object);
    ASSERT_EQ(obj->refcount, start_ref + 2);

    aspace_destroy(a);
    aspace_destroy(b);
    vm_object_release(obj);
    printf("Passed aspace_shared_object_two_spaces\n");
}

TEST(aspace_clone_copies_metadata) {
    printf("Running aspace_clone_copies_metadata...\n");
    address_space_t *src = NULL, *dst = NULL;
    ASSERT_EQ(0, aspace_create(&src, 0));

    vm_object_t *anon = vm_object_create_anon(0x8000, 0);
    vm_object_t *shared = vm_object_create_shared(0x4000, 0);

    ASSERT_EQ(0, aspace_region_attach(src, 0x400000, 0x3000, VM_PROT_READ|VM_PROT_WRITE, 0,
                                      VM_INHERIT_COPY_META, anon, 0x1000, NULL));
    ASSERT_EQ(0, aspace_region_attach(src, 0x500000, 0x2000, VM_PROT_READ, 0,
                                      VM_INHERIT_SHARE, shared, 0, NULL));

    ASSERT_EQ(0, aspace_clone(src, &dst, 0));

    vm_region_t *r1 = aspace_lookup_region(dst, 0x400100);
    vm_region_t *r2 = aspace_lookup_region(dst, 0x500100);

    ASSERT_NOT_NULL(r1);
    ASSERT_NOT_NULL(r2);
    ASSERT_EQ(r1->base, 0x400000u);
    ASSERT_EQ(r1->length, 0x3000u);
    ASSERT_EQ(r1->object, anon);
    ASSERT_EQ(r1->object_offset, 0x1000u);

    ASSERT_EQ(r2->object, shared);

    aspace_destroy(dst);
    aspace_destroy(src);
    vm_object_release(anon);
    vm_object_release(shared);
    printf("Passed aspace_clone_copies_metadata\n");
}

TEST(aspace_teardown_releases_refs) {
    printf("Running aspace_teardown_releases_refs...\n");
    address_space_t *as = NULL, *clone = NULL;
    ASSERT_EQ(0, aspace_create(&as, 0));

    vm_object_t *obj = vm_object_create_shared(0x4000, 0);
    ASSERT_NOT_NULL(obj);
    ASSERT_EQ(obj->refcount, 1u);

    ASSERT_EQ(0, aspace_region_attach(as, 0x600000, 0x2000, VM_PROT_READ, 0,
                                      VM_INHERIT_SHARE, obj, 0, NULL));
    ASSERT_EQ(obj->refcount, 2u);

    ASSERT_EQ(0, aspace_clone(as, &clone, 0));
    ASSERT_EQ(obj->refcount, 3u);

    aspace_destroy(clone);
    ASSERT_EQ(obj->refcount, 2u);

    aspace_destroy(as);
    ASSERT_EQ(obj->refcount, 1u);

    vm_object_release(obj);
    printf("Passed aspace_teardown_releases_refs\n");
}

TEST(aspace_lookup_authoritative_without_pt_mapping) {
    printf("Running aspace_lookup_authoritative_without_pt_mapping...\n");
    address_space_t *as = NULL;
    ASSERT_EQ(0, aspace_create(&as, 0));

    vm_object_t *obj = vm_object_create_anon(0x4000, 0);

    ASSERT_EQ(0, aspace_region_attach(as, 0x700000, 0x2000, VM_PROT_READ|VM_PROT_WRITE, 0,
                                      VM_INHERIT_COPY_META, obj, 0x100, NULL));

    vm_region_t *r = aspace_lookup_region(as, 0x700100);
    uint64_t off = 0;
    vm_object_t *found = aspace_lookup_object(as, 0x700100, NULL, &off);

    ASSERT_NOT_NULL(r);
    ASSERT_EQ(found, obj);
    ASSERT_EQ(off, 0x200u); // 0x100 (offset) + 0x100 (va - base)

    aspace_destroy(as);
    vm_object_release(obj);
    printf("Passed aspace_lookup_authoritative_without_pt_mapping\n");
}

TEST(vm_object_kinds_basic_construction) {
    printf("Running vm_object_kinds_basic_construction...\n");

    int fake_backing = 42;

    // Test invalid unaligned size
    vm_object_t *invalid_obj = vm_object_create_anon(0x100, 0);
    ASSERT_EQ(invalid_obj, NULL);

    vm_object_t *file_obj = vm_object_create_file(&fake_backing, 0x2000, 0x4000, 0);
    ASSERT_NOT_NULL(file_obj);
    ASSERT_EQ(file_obj->kind, VM_OBJECT_FILE);
    ASSERT_EQ(file_obj->u.file.file_offset, 0x2000u);
    ASSERT_NOT_NULL((void*)file_obj->ops); // Explicit ops test

    vm_object_t *dev_obj = vm_object_create_device(0x900000, 0x2000, 0, 0);
    ASSERT_NOT_NULL(dev_obj);
    ASSERT_EQ(dev_obj->kind, VM_OBJECT_DEVICE);
    ASSERT_EQ(dev_obj->u.device.phys_base, 0x900000u);
    ASSERT_NOT_NULL((void*)dev_obj->ops);

    vm_object_t *dma_obj = vm_object_create_dma(0xA00000, 0x3000, 1, 0, 0);
    ASSERT_NOT_NULL(dma_obj);
    ASSERT_EQ(dma_obj->kind, VM_OBJECT_DMA);
    ASSERT_EQ(dma_obj->u.dma.phys_base, 0xA00000u);
    ASSERT_NOT_NULL((void*)dma_obj->ops);

    vm_object_release(file_obj);
    vm_object_release(dev_obj);
    vm_object_release(dma_obj);

    printf("Passed vm_object_kinds_basic_construction\n");
}

TEST(vm_object_lifecycle_hardening) {
    printf("Running vm_object_lifecycle_hardening...\n");

    vm_object_t *obj = vm_object_create_anon(0x4000, 0);
    ASSERT_NOT_NULL(obj);
    ASSERT_EQ(obj->refcount, 1u);
    ASSERT_EQ(obj->magic, VM_OBJECT_MAGIC_ALIVE);

    vm_object_retain(obj);
    ASSERT_EQ(obj->refcount, 2u);

    vm_object_release(obj);
    ASSERT_EQ(obj->refcount, 1u);

    // Final release
    vm_object_release(obj);
    // At this point obj is kfree'd, can't reliably read magic unless we intercept kfree.
    // But we know it didn't trap/panic.

    printf("Passed vm_object_lifecycle_hardening\n");
}

// Mocks for mm_model and aspace_profile to test create constraints
mem_model_t mock_mem_model = MEM_MODEL_MMU_FULL;
// aspace_profile_t mock_aspace_profile = ASPACE_PROFILE_FULL; // Removed since we map directly from mem_model

mem_model_t mem_model_get_current(void) {
    return mock_mem_model;
}

TEST(aspace_create_profile_enforcement) {
    printf("Running aspace_create_profile_enforcement...\n");
    address_space_t *as = NULL;

    // 1. FULL + rich create (flags=1) -> PASS
    mock_mem_model = MEM_MODEL_MMU_FULL;
    ASSERT_EQ(0, aspace_create(&as, 1));
    aspace_destroy(as);

    // 2. FULL + basic create (flags=0) -> PASS
    mock_mem_model = MEM_MODEL_MMU_FULL;
    ASSERT_EQ(0, aspace_create(&as, 0));
    aspace_destroy(as);

    // 3. REGION_ONLY + rich create -> FAIL
    mock_mem_model = MEM_MODEL_MPU; // implies REGION_ONLY
    ASSERT_NE(0, aspace_create(&as, 1));

    // 4. REGION_ONLY + basic create -> PASS
    mock_mem_model = MEM_MODEL_MPU; // implies REGION_ONLY
    ASSERT_EQ(0, aspace_create(&as, 0));
    aspace_destroy(as);

    // 5. FLAT + rich create -> FAIL
    // For tests, we use MMU_LITE which implies SPLIT in aspace_profile_get_for_model
    // We cannot easily inject FLAT here without changing aspace_profile.h, but we can test
    // that constrained profiles block rich. Since LITE defaults to SPLIT and we left SPLIT permissive,
    // this test for FLAT would need us to be able to mock the profile directly, which is inline.
    // Let's rely on MPU -> REGION_ONLY test for the rejection path logic.

    printf("Passed aspace_create_profile_enforcement\n");
}

int main() {
    hal_pt_init(); // Needs mock
    extern void prot_domain_init(void);
    prot_domain_init();

    aspace_overlap_rejected();
    aspace_shared_object_two_spaces();
    aspace_clone_copies_metadata();
    aspace_teardown_releases_refs();
    aspace_lookup_authoritative_without_pt_mapping();
    vm_object_kinds_basic_construction();
    vm_object_lifecycle_hardening();
    aspace_create_profile_enforcement();

    printf("All ASPACE tests passed successfully!\n");
    return 0;
}

// Temporary additions for the new page fault features
#include "../kernel/include/mm/fault.h"
#include "../kernel/include/hal/hal_pt.h"
#include "../kernel/include/hal/hal.h"
