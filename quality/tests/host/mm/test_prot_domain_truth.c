#include <mm/prot_domain.h>
#include <hal/hal_pt.h>
#include <hal/hal_mpu.h>
#include <hal/hal_mm.h>
#include <kernel/status.h>
#include <mm/vm_mapping.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Mock memory management
void* kmalloc(size_t s) {
    return malloc(s);
}
void kfree(void* p) {
    free(p);
}

// Global active caps
static hal_mm_backend_kind_t mock_backend_kind = HAL_MM_BACKEND_NONE;
void hal_mm_backend_caps(hal_mm_backend_caps_t *out) {
    memset(out, 0, sizeof(*out));
    out->kind = mock_backend_kind;
    out->max_regions = 16;
}

// Track PT calls
static int pt_create_calls = 0;
static int pt_destroy_calls = 0;
static int pt_map_calls = 0;
static int pt_unmap_calls = 0;
static int pt_protect_calls = 0;
static int pt_query_calls = 0;

static int pt_inject_ret = 0;

phys_addr_t vmm_get_kernel_root(void) {
    return 0x5000;
}

static phys_addr_t mock_create_address_space(phys_addr_t kernel_root_table) {
    pt_create_calls++;
    return 0x9000;
}
static void mock_destroy_address_space(phys_addr_t root_pt) {
    pt_destroy_calls++;
}
static int mock_map_range(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, size_t size, uint32_t flags) {
    pt_map_calls++;
    return pt_inject_ret;
}
static int mock_unmap_range(phys_addr_t root_pt, virt_addr_t vaddr, size_t size) {
    pt_unmap_calls++;
    return pt_inject_ret;
}
static int mock_protect_range(phys_addr_t root_pt, virt_addr_t vaddr, size_t size, uint32_t new_flags) {
    pt_protect_calls++;
    return pt_inject_ret;
}
static int mock_query_mapping(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, size_t *mapped_size, uint32_t *flags) {
    pt_query_calls++;
    if (paddr) *paddr = 0x8000;
    return pt_inject_ret;
}

phys_addr_t hal_pt_create_address_space(phys_addr_t kernel_root_table) {
    if (active_hal_pt && active_hal_pt->create_address_space) {
        return active_hal_pt->create_address_space(kernel_root_table);
    }
    return 0;
}
void hal_pt_destroy_address_space(phys_addr_t root_pt) {
    if (active_hal_pt && active_hal_pt->destroy_address_space) {
        active_hal_pt->destroy_address_space(root_pt);
    }
}
int hal_pt_map_range(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, size_t size, uint32_t flags) {
    if (active_hal_pt && active_hal_pt->map_range) {
        return active_hal_pt->map_range(root_pt, vaddr, paddr, size, flags);
    }
    return -1;
}
int hal_pt_unmap_range(phys_addr_t root_pt, virt_addr_t vaddr, size_t size) {
    if (active_hal_pt && active_hal_pt->unmap_range) {
        return active_hal_pt->unmap_range(root_pt, vaddr, size);
    }
    return -1;
}
int hal_pt_protect_range(phys_addr_t root_pt, virt_addr_t vaddr, size_t size, uint32_t new_flags) {
    if (active_hal_pt && active_hal_pt->protect_range) {
        return active_hal_pt->protect_range(root_pt, vaddr, size, new_flags);
    }
    return -1;
}
int hal_pt_query_mapping(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, size_t *mapped_size, uint32_t *flags) {
    if (active_hal_pt && active_hal_pt->query_mapping) {
        return active_hal_pt->query_mapping(root_pt, vaddr, paddr, mapped_size, flags);
    }
    return -1;
}

static hal_pt_ops_t mock_hal_pt_ops = {
    .create_address_space = mock_create_address_space,
    .destroy_address_space = mock_destroy_address_space,
    .map_range = mock_map_range,
    .unmap_range = mock_unmap_range,
    .protect_range = mock_protect_range,
    .query_mapping = mock_query_mapping,
};

hal_pt_ops_t *active_hal_pt = &mock_hal_pt_ops;

void hal_pt_init(void) {}

// Track MPU calls
static int mpu_program_calls = 0;
static int mpu_disable_calls = 0;
static int mpu_caps_calls = 0;

static int mpu_inject_ret = 0;

static int mock_mpu_program_region(uint32_t region_id, phys_addr_t base, size_t size, uint32_t flags) {
    mpu_program_calls++;
    return mpu_inject_ret;
}
static int mock_mpu_disable_region(uint32_t region_id) {
    mpu_disable_calls++;
    return mpu_inject_ret;
}
static const hal_mpu_caps_t mock_mpu_caps = {
    .max_regions = 16,
    .supports_subregions = false,
    .requires_power_of_two_size = false,
    .min_region_alignment = 32
};
static const hal_mpu_caps_t* mock_mpu_get_caps(void) {
    mpu_caps_calls++;
    return &mock_mpu_caps;
}

static hal_mpu_ops_t mock_hal_mpu_ops = {
    .program_region = mock_mpu_program_region,
    .disable_region = mock_mpu_disable_region,
    .get_caps = mock_mpu_get_caps,
};

// Reset trackers
static void reset_counters(void) {
    pt_create_calls = 0;
    pt_destroy_calls = 0;
    pt_map_calls = 0;
    pt_unmap_calls = 0;
    pt_protect_calls = 0;
    pt_query_calls = 0;
    pt_inject_ret = 0;

    mpu_program_calls = 0;
    mpu_disable_calls = 0;
    mpu_caps_calls = 0;
    mpu_inject_ret = 0;
}

void test_mmu_full_truth(void) {
    printf("Running test_mmu_full_truth...\n");
    reset_counters();
    mock_backend_kind = HAL_MM_BACKEND_MMU_FULL;
    prot_domain_init();

    prot_domain_t* domain = NULL;
    int ret = prot_domain_create(&domain);
    assert(ret == K_OK);
    assert(domain != NULL);
    assert(domain->mode == PROT_MODE_MMU_FULL);
    assert(pt_create_calls == 1);

    // Test map
    ret = prot_domain_map_region(domain, 0x1000, 0x2000, 4096, 0);
    assert(ret == K_OK);
    assert(pt_map_calls == 1);

    // Test fail propagation
    pt_inject_ret = K_ERR_DENIED;
    ret = prot_domain_map_region(domain, 0x3000, 0x4000, 4096, 0);
    assert(ret == K_ERR_DENIED);
    assert(pt_map_calls == 2);

    // Test unmap
    pt_inject_ret = K_OK;
    ret = prot_domain_unmap_region(domain, 0x1000, 4096);
    assert(ret == K_OK);
    assert(pt_unmap_calls == 1);

    // Test protect
    ret = prot_domain_protect_region(domain, 0x1000, 4096, 0);
    assert(ret == K_OK);
    assert(pt_protect_calls == 1);

    // Test query
    uintptr_t pa = 0;
    ret = prot_domain_query_region(domain, 0x1000, &pa, NULL);
    assert(ret == K_OK);
    assert(pa == 0x8000);
    assert(pt_query_calls == 1);

    prot_domain_destroy(domain);
    assert(pt_destroy_calls == 1);
    printf("test_mmu_full_truth passed!\n");
}

void test_mmu_lite_truth(void) {
    printf("Running test_mmu_lite_truth...\n");
    reset_counters();
    mock_backend_kind = HAL_MM_BACKEND_MMU_LITE;
    prot_domain_init();

    prot_domain_t* domain = NULL;
    int ret = prot_domain_create(&domain);
    assert(ret == K_OK);
    assert(domain != NULL);
    assert(domain->mode == PROT_MODE_MMU_LITE);

    // Test map with VM_MAP_EXEC_OK
    ret = prot_domain_map_region(domain, 0x1000, 0x2000, 4096, VM_MAP_EXEC_OK);
    assert(ret == K_OK);
    assert(pt_map_calls == 1); // Real backend was called

    // Test alignment checks
    ret = prot_domain_map_region(domain, 0x1001, 0x2000, 4096, 0);
    assert(ret == K_ERR_INVALID_ARG);
    assert(pt_map_calls == 1); // No backend call for bad arguments

    // Unmap
    ret = prot_domain_unmap_region(domain, 0x1000, 4096);
    assert(ret == K_OK);
    assert(pt_unmap_calls == 1);

    // Protect
    ret = prot_domain_protect_region(domain, 0x1000, 4096, 0);
    assert(ret == K_OK);
    assert(pt_protect_calls == 1);

    // Invariant: no success without backend call
    reset_counters();
    active_hal_pt = NULL; // Unregister PT ops to simulate missing backend
    ret = prot_domain_map_region(domain, 0x1000, 0x2000, 4096, 0);
    assert(ret == K_ERR_UNSUPPORTED);
    assert(pt_map_calls == 0);

    active_hal_pt = &mock_hal_pt_ops;
    prot_domain_destroy(domain);
    printf("test_mmu_lite_truth passed!\n");
}

void test_mpu_truth(void) {
    printf("Running test_mpu_truth...\n");
    reset_counters();
    mock_backend_kind = HAL_MM_BACKEND_MPU_ONLY;
    hal_mpu_register_ops(&mock_hal_mpu_ops);
    prot_domain_init();

    prot_domain_t* domain = NULL;
    int ret = prot_domain_create(&domain);
    assert(ret == K_OK);
    assert(domain != NULL);
    assert(domain->mode == PROT_MODE_MPU_ONLY);

    // Test map region
    ret = prot_domain_map_region(domain, 0x1000, 0x1000, 4096, 0);
    assert(ret == K_OK);
    assert(mpu_program_calls == 1);
    assert(pt_map_calls == 0); // No PT operations invoked for MPU

    // Test overlap rejection
    ret = prot_domain_map_region(domain, 0x1000, 0x1000, 4096, 0);
    assert(ret == K_ERR_VM_ALREADY_MAPPED);
    assert(mpu_program_calls == 1); // Not called since rejected early

    // Test protect region
    ret = prot_domain_protect_region(domain, 0x1000, 4096, 1);
    assert(ret == K_OK);
    assert(mpu_program_calls == 2);

    // Test query region
    uintptr_t pa = 0;
    uint32_t flags = 0;
    ret = prot_domain_query_region(domain, 0x1000, &pa, &flags);
    assert(ret == K_OK);
    assert(pa == 0x1000);
    assert(flags == 1);

    // Test unmap region
    ret = prot_domain_unmap_region(domain, 0x1000, 4096);
    assert(ret == K_OK);
    assert(mpu_disable_calls == 1);

    // Test fail-closed when active_hal_mpu is removed
    active_hal_mpu = NULL;
    ret = prot_domain_map_region(domain, 0x1000, 0x1000, 4096, 0);
    assert(ret == K_ERR_UNSUPPORTED);
    assert(mpu_program_calls == 2); // No new program call

    active_hal_mpu = &mock_hal_mpu_ops;
    prot_domain_destroy(domain);
    printf("test_mpu_truth passed!\n");
}

void test_dispatch_invariance(void) {
    printf("Running test_dispatch_invariance...\n");
    reset_counters();

    // 1. Create a MMU_FULL domain
    mock_backend_kind = HAL_MM_BACKEND_MMU_FULL;
    prot_domain_init();
    prot_domain_t* dom_full = NULL;
    int ret = prot_domain_create(&dom_full);
    assert(ret == K_OK);

    // 2. Change active backend to MMU_LITE
    mock_backend_kind = HAL_MM_BACKEND_MMU_LITE;
    prot_domain_init();
    prot_domain_t* dom_lite = NULL;
    ret = prot_domain_create(&dom_lite);
    assert(ret == K_OK);

    // 3. Verify they still route to their own specific backend_ops
    assert(dom_full->ops->map_region != dom_lite->ops->map_region);

    ret = prot_domain_map_region(dom_full, 0x1000, 0x2000, 4096, 0);
    assert(ret == K_OK);
    // dom_full should have routed via mmu_full ops
    assert(dom_full->mode == PROT_MODE_MMU_FULL);

    prot_domain_destroy(dom_full);
    prot_domain_destroy(dom_lite);
    printf("test_dispatch_invariance passed!\n");
}

int main(void) {
    test_mmu_full_truth();
    test_mmu_lite_truth();
    test_mpu_truth();
    test_dispatch_invariance();
    printf("All backend-truth protection domain tests passed successfully!\n");
    return 0;
}
