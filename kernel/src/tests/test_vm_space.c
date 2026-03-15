#include "../../include/tests/test_framework.h"
#include "../../include/mm/vm_space.h"
#include <stdlib.h>

uint32_t hal_get_core_id_mock(void) {
    return 0;
}
#include "../../include/mm/vm_mapping.h"
#include "../../include/mm/arch_vm.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Mock monitor operations
int mon_vm_send_map(vm_space_t *space, const vm_map_req_t *req, bool strict) { return 0; }
int mon_vm_send_unmap(vm_space_t *space, uintptr_t va, size_t len, bool strict) { return 0; }
int mon_vm_send_protect(vm_space_t *space, uintptr_t va, size_t len, uint64_t prot, uint64_t mem_type, bool strict) { return 0; }
int mon_vm_send_tlb_invalidate_range(vm_space_t *space, uintptr_t va, size_t len, bool strict) { return 0; }
int mon_vm_send_prepare_rt(vm_space_t *space, uint32_t target_core_id) { return 0; }
int mon_vm_wait_for_acks(uint64_t txn_id, cpu_mask_t required_cores) { return 0; }

// Mock active arch ops for testing
static int mock_space_init(vm_space_t *space, vm_core_state_t *local) {
    local->state = VM_REALIZE_VALID;
    return 0;
}
static int mock_map(vm_space_t *space, vm_core_state_t *local, uintptr_t va, uintptr_t pa, size_t len, uint64_t prot, uint64_t mem_type, uint64_t flags) {
    return 0;
}
static int mock_activate(vm_space_t *space, vm_core_state_t *local) {
    return 0;
}
static arch_vm_ops_t mock_ops = {
    .space_init = mock_space_init,
    .map = mock_map,
    .activate = mock_activate
};

void test_vm_space_creation(void) {
    active_arch_vm_ops = &mock_ops;

    vm_space_t *space = NULL;
    int ret = vm_space_create(&space, MEM_PROFILE_MMU_BASIC, VM_TIMING_BEST_EFFORT);

    TEST_ASSERT(ret == 0);
    TEST_ASSERT(space != NULL);
    TEST_ASSERT(space->generation == 1);
    TEST_ASSERT(space->profile == MEM_PROFILE_MMU_BASIC);
    TEST_ASSERT(space->timing_class == VM_TIMING_BEST_EFFORT);
    TEST_ASSERT(space->mappings.head == NULL);

    vm_space_destroy(space);
}

void test_vm_mapping_lazy(void) {
    active_arch_vm_ops = &mock_ops;

    vm_space_t *space = NULL;
    vm_space_create(&space, MEM_PROFILE_MMU_BASIC, VM_TIMING_BEST_EFFORT);

    vm_map_req_t req = {
        .va = 0x1000,
        .pa = 0x2000,
        .len = 4096,
        .prot = VM_PROT_READ | VM_PROT_WRITE,
        .mem_type = VM_MEM_NORMAL,
        .map_flags = 0,
        .zone = VM_MEM_ZONE_NORMAL,
        .timing_class = VM_TIMING_BEST_EFFORT
    };

    uint64_t old_gen = space->generation;
    int ret = vm_map(space, &req);

    TEST_ASSERT(ret == 0);
    TEST_ASSERT(space->generation == old_gen + 1);
    TEST_ASSERT(space->mappings.head != NULL);
    TEST_ASSERT(space->mappings.head->va_start == 0x1000);
    TEST_ASSERT(space->mappings.head->map_gen == space->generation);

    vm_space_destroy(space);
}

void test_vm_rt_firm_constraint(void) {
    active_arch_vm_ops = &mock_ops;

    vm_space_t *space = NULL;
    vm_space_create(&space, MEM_PROFILE_MMU_BASIC, VM_TIMING_FIRM_RT);

    // Should NOT allow lazy realize by default
    TEST_ASSERT(space->allow_lazy_realize == false);

    // Need explicit prepare
    int ret = vm_prepare_rt_core(space, 0); // Prepare core 0
    TEST_ASSERT(ret == 0);
    TEST_ASSERT((space->rt_ready_cores & 1) != 0);

    vm_space_destroy(space);
}

int main(void) {
    test_vm_space_creation();
    test_vm_mapping_lazy();
    test_vm_rt_firm_constraint();
    printf("All VM space tests passed!\n");
    return 0; // Success
}
