#include <hal/hal_pt.h>
#include <mm/mem_model.h>
#include <mm/vm_space.h>
#include <mm/aspace.h>
#include <mm/arch_vm.h>
#include <bharat/cpu_local.h>
#include <kernel/status.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Mock dependencies
void hal_pt_init(void) {}
struct prot_domain* prot_domain_create(void) { return NULL; }
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

mem_model_t mem_model_get_current(void) { return MEM_MODEL_MMU_FULL; }
uint64_t mem_model_get_caps(void) { return ~0ULL; }
bool arch_has_cap(uint32_t cap) { return true; }
void vm_object_release(vm_object_t *obj) { (void)obj; }
void vm_object_retain(vm_object_t *obj) { (void)obj; }
void* kmalloc(size_t s) { return malloc(s); }
void kfree(void* p) { free(p); }
void console_log(const char* fmt, ...) { (void)fmt; }
void kernel_panic(const char* m) { printf("PANIC: %s\n", m); exit(1); }
uint64_t arch_get_caps(void) { return 0; }

// cpu locals mock
cpu_local_t g_cpu_locals[MAX_CPUS];
uint32_t hal_cpu_get_id(void) { return 0; }

// mock active arch ops
static int mock_space_init(vm_space_t *space, vm_core_state_t *local) { return 0; }
static int mock_map(vm_space_t *space, vm_core_state_t *local, uintptr_t va, uintptr_t pa, size_t len, uint64_t prot, uint64_t mem_type, uint64_t flags) { return 0; }
static int mock_activate(vm_space_t *space, vm_core_state_t *local) { return 0; }
static arch_vm_ops_t mock_ops = {
    .space_init = mock_space_init,
    .map = mock_map,
    .activate = mock_activate
};

// Mock monitor operations
int mon_vm_send_map(vm_space_t *space, const vm_map_req_t *req, bool strict) { return 0; }
int mon_vm_send_unmap(vm_space_t *space, uintptr_t va, size_t len, bool strict) { return 0; }
int mon_vm_send_protect(vm_space_t *space, uintptr_t va, size_t len, uint64_t prot, uint64_t mem_type, bool strict) { return 0; }

// mm_switch_active_aspace mock
void mm_switch_active_aspace(uint32_t core_id, address_space_t *prev_as, address_space_t *next_as) {
    g_cpu_locals[core_id].current_as = next_as;
    if (prev_as) {
        aspace_deactivate_on_cpu(prev_as, core_id);
    }
    if (next_as) {
        aspace_activate_on_cpu(next_as, core_id);
    }
}

void test_vm_space_ownership_and_lifecycle(void) {
    printf("Running test_vm_space_ownership_and_lifecycle...\n");
    active_arch_vm_ops = &mock_ops;

    vm_space_t *space = NULL;
    int ret = vm_space_create(&space, MEM_PROFILE_MMU_BASIC, VM_TIMING_BEST_EFFORT);
    assert(ret == 0);
    assert(space != NULL);
    assert(space->aspace != NULL);
    assert(space->aspace->state == ASPACE_STATE_CREATED);
    assert(space->aspace->timing_class == VM_TIMING_BEST_EFFORT);

    // Verify 1:1 linked and not casting punned
    assert((void*)space != (void*)space->aspace);

    // Activate CPU 0
    g_cpu_locals[0].current_as = NULL;
    ret = vm_activate_local(space);
    assert(ret == 0);

    // cpu_local.current_as and address_space_t.active_mask must be authoritative
    assert(g_cpu_locals[0].current_as == space->aspace);
    assert((space->aspace->active_mask & (1ULL << 0)) != 0);

    // Legacy/compatibility fields should match or sync but not decide
    assert((space->active_cores & (1ULL << 0)) != 0);

    // Destroy should cleanly tear down owned aspace
    vm_space_destroy(space);
    printf("test_vm_space_ownership_and_lifecycle passed!\n");
}

int main(void) {
    test_vm_space_ownership_and_lifecycle();
    printf("All memory authority tests passed successfully!\n");
    return 0;
}
