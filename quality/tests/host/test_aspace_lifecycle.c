#include <mm/aspace.h>
#include <kernel/status.h>
#include <mm/aspace_profile.h>
#include <mm/mem_model.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Mock dependencies
void hal_pt_init(void) {}
struct prot_domain* prot_domain_create(void) { return NULL; }
void prot_domain_destroy(struct prot_domain* pd) { (void)pd; }
void mm_stats_inc_aspace_create_calls(void) {}
phys_addr_t vmm_get_kernel_root(void) { return 0; }
void* active_hal_pt = NULL;

typedef struct {
    uint64_t aspace_create_calls;
    uint64_t aspace_rejected_by_profile;
    uint64_t aspace_create_failures;
} mm_stats_t;
mm_stats_t mm_stats;

mem_model_t mem_model_get_current(void) { return MEM_MODEL_MMU_FULL; }
bool arch_has_cap(uint32_t cap) { return true; }
void vm_object_release(vm_object_t *obj) { (void)obj; }
void vm_object_retain(vm_object_t *obj) { (void)obj; }
void* kmalloc(size_t s) { return malloc(s); }
void kfree(void* p) { free(p); }
void console_log(const char* fmt, ...) { (void)fmt; }
void kernel_panic(const char* m) { printf("PANIC: %s\n", m); exit(1); }
uint64_t arch_get_caps(void) { return 0; }

void test_aspace_lifecycle(void) {
    printf("Running test_aspace_lifecycle...\n");
    address_space_t *as = NULL;
    assert(aspace_create(&as, 0) == K_OK);
    assert(as != NULL);
    assert(as->state == ASPACE_STATE_CREATED);
    assert(as->active_mask == 0);

    // Test activate
    assert(aspace_activate_on_cpu(as, 0) == K_OK);
    assert(as->state == ASPACE_STATE_ACTIVE);
    assert(as->active_mask == (1ULL << 0));

    assert(aspace_activate_on_cpu(as, 1) == K_OK);
    assert(as->active_mask == (1ULL << 0) | (1ULL << 1));

    // Test deactivate
    assert(aspace_deactivate_on_cpu(as, 0) == K_OK);
    assert(as->active_mask == (1ULL << 1));

    // Test validity
    assert(aspace_is_valid_for_tlb(as) == true);

    // Test generation
    uint64_t gen = as->tlb_gen;
    assert(aspace_next_tlb_generation(as) == gen + 1);

    // Negative tests: DYING state
    as->state = ASPACE_STATE_DYING;
    assert(aspace_activate_on_cpu(as, 2) == K_ERR_BAD_STATE);
    assert(aspace_region_reserve(as, 0x2000, 0x1000, 0, 0, 0, NULL) == K_ERR_BAD_STATE);
    assert(aspace_region_attach(as, 0x3000, 0x1000, 0, 0, 0, NULL, 0, NULL) == K_ERR_BAD_STATE);
    assert(aspace_region_detach(as, 0x1000) == K_ERR_BAD_STATE);
    address_space_t *dummy_clone = NULL;
    assert(aspace_clone(as, &dummy_clone, 0) == K_ERR_BAD_STATE);
    assert(aspace_map_region(as, 0x4000, 0x1000, 0, 0, NULL, 0, NULL) == K_ERR_BAD_STATE);
    assert(aspace_unmap_region(as, 0x4000, 0x1000) == K_ERR_BAD_STATE);
    assert(aspace_protect_region(as, 0x4000, 0x1000, 0) == K_ERR_BAD_STATE);

    // DESTROYED state
    as->state = ASPACE_STATE_DESTROYED;
    assert(aspace_activate_on_cpu(as, 2) == K_ERR_BAD_STATE);
    assert(aspace_region_reserve(as, 0x2000, 0x1000, 0, 0, 0, NULL) == K_ERR_BAD_STATE);
}

int main(void) {
    test_aspace_lifecycle();
    printf("All aspace_lifecycle host tests passed!\n");
    return 0;
}
