#include "pmm_lifecycle.h"
#include "bharat/kernel/ds/bh_refcount.h"
#include <assert.h>
#include <stdio.h>

// Mock kernel_panic for host tests
void kernel_panic(const char *msg) {
    fprintf(stderr, "KERNEL PANIC: %s\n", msg);
}

// Mock hal_cpu_get_id
uint32_t hal_cpu_get_id() { return 0; }

void test_page_lifecycle_basic() {
    page_t page = {0};
    bh_refcount_init(&page.ref_count, 0);
    page.state = PMM_PAGE_STATE_FREE;

    assert(pmm_page_is_allocatable(&page));
    assert(!pmm_page_is_pinned(&page));

    // Transition FREE -> ALLOCATED
    kstatus_t status = pmm_page_transition(&page, PMM_PAGE_STATE_FREE, PMM_PAGE_STATE_ALLOCATED);
    assert(status == K_OK);
    assert(page.state == PMM_PAGE_STATE_ALLOCATED);
    assert(!pmm_page_is_allocatable(&page));

    // Attempt invalid transition ALLOCATED -> FREE (should use proper transition)
    status = pmm_page_transition(&page, PMM_PAGE_STATE_FREE, PMM_PAGE_STATE_ALLOCATED);
    assert(status == K_ERR_BAD_STATE);

    // Refcount tests
    bh_refcount_init(&page.ref_count, 1);
    status = pmm_page_get(&page);
    assert(status == K_OK);
    assert(bh_refcount_read(&page.ref_count) == 2);

    bool is_last = false;
    status = pmm_page_put(&page, &is_last);
    assert(status == K_OK);
    assert(!is_last);
    assert(bh_refcount_read(&page.ref_count) == 1);

    status = pmm_page_put(&page, &is_last);
    assert(status == K_OK);
    assert(is_last);
    assert(bh_refcount_read(&page.ref_count) == 0);

    printf("test_page_lifecycle_basic passed\n");
}

void test_page_lifecycle_pinning() {
    page_t page = {0};
    page.pin_count = 1;
    assert(pmm_page_is_pinned(&page));
    assert(!pmm_page_can_free(&page));

    page.pin_count = 0;
    assert(!pmm_page_is_pinned(&page));
    assert(pmm_page_can_free(&page));

    printf("test_page_lifecycle_pinning passed\n");
}

int main() {
    test_page_lifecycle_basic();
    test_page_lifecycle_pinning();
    printf("All PMM lifecycle tests passed!\n");
    return 0;
}
