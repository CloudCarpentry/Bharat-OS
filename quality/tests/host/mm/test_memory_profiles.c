#include <mm/aspace.h>
#include <mm/mem_model.h>
#include <mm/aspace_profile.h>
#include <hal/hal_pt.h>
#include <kernel/status.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Mock dependencies
void hal_pt_init(void) {}
int prot_domain_create(struct prot_domain** out) { *out = NULL; return 0; }
void prot_domain_destroy(struct prot_domain* pd) { (void)pd; }
void mm_stats_inc_aspace_create_calls(void) {}
phys_addr_t vmm_get_kernel_root(void) { return 0; }

static phys_addr_t mock_create_address_space(phys_addr_t kernel_root_table) {
    return 0x9000;
}
static void mock_destroy_address_space(phys_addr_t root_pt) {
    (void)root_pt;
}
static hal_pt_ops_t mock_hal_pt = {
    .create_address_space = mock_create_address_space,
    .destroy_address_space = mock_destroy_address_space
};
hal_pt_ops_t* active_hal_pt = &mock_hal_pt;

typedef struct {
    uint64_t aspace_create_calls;
    uint64_t aspace_rejected_by_profile;
    uint64_t aspace_create_failures;
} mm_stats_t;
mm_stats_t mm_stats;

static mem_model_t mock_model = MEM_MODEL_MMU_FULL;
mem_model_t mem_model_get_current(void) { return mock_model; }

bool arch_has_cap(uint32_t cap) { return true; }
void vm_object_release(vm_object_t *obj) { (void)obj; }
void vm_object_retain(vm_object_t *obj) { (void)obj; }
void* kmalloc(size_t s) { return malloc(s); }
void kfree(void* p) { free(p); }
void console_log(const char* fmt, ...) { (void)fmt; }
void kernel_panic(const char* m) { printf("PANIC: %s\n", m); exit(1); }
uint64_t arch_get_caps(void) { return 0; }

void test_memory_profile_matrix(void) {
    printf("Running test_memory_profile_matrix...\n");

    // Case 1: MMU_FULL
    mock_model = MEM_MODEL_MMU_FULL;
    address_space_t *as_full = NULL;
    int ret = aspace_create(&as_full, 0);
    assert(ret == K_OK);
    assert(as_full != NULL);
    assert(as_full->root_pt != 0); // MMU_FULL allocates page table root
    aspace_destroy(as_full);

    // Case 2: MPU / REGION_ONLY
    mock_model = MEM_MODEL_MPU;
    address_space_t *as_mpu = NULL;
    ret = aspace_create(&as_mpu, 0);
    assert(ret == K_OK);
    assert(as_mpu != NULL);
    assert(as_mpu->root_pt == 0); // MPU must strictly bypass page table root allocation
    aspace_destroy(as_mpu);

    // Case 3: Incompatible profile combinations (rich flags under MPU)
    // Non-zero flags imply rich VM / demand paging semantics
    ret = aspace_create(&as_mpu, 1);
    assert(ret == K_ERR_PROFILE_RESTRICTED);

    printf("test_memory_profile_matrix passed!\n");
}

int main(void) {
    test_memory_profile_matrix();
    printf("All memory profile matrix tests passed successfully!\n");
    return 0;
}
