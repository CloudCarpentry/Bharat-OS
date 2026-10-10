#include "../../kernel/include/mm/physmap.h"
#include "../../kernel/include/kernel/status.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define SIM_MEMORY_SIZE (8192) // 2 pages
static uint8_t sim_memory[SIM_MEMORY_SIZE];
#define SIM_BASE_ADDR 0x80000000ULL

// Mock hal_translate_ops
static bool mock_has_linear_physmap(void) { return true; }

static void* mock_phys_to_virt(phys_addr_t phys) {
    if (phys >= SIM_BASE_ADDR && phys < SIM_BASE_ADDR + SIM_MEMORY_SIZE) {
        return (void*)&sim_memory[phys - SIM_BASE_ADDR];
    }
    return NULL;
}

static phys_addr_t mock_virt_to_phys(const void* virt) {
    uint8_t *v = (uint8_t*)virt;
    if (v >= sim_memory && v < sim_memory + SIM_MEMORY_SIZE) {
        return SIM_BASE_ADDR + (v - sim_memory);
    }
    return 0;
}

static translate_backend_kind_t mock_backend_type(void) { return TRANSLATE_BACKEND_MMU; }
static translate_exec_class_t mock_exec_class(void) { return TRANSLATE_EXEC_MMU_FULL; }

static const hal_translate_ops_t mock_ops = {
    .backend_type = mock_backend_type,
    .exec_class = mock_exec_class,
    .phys_to_virt = mock_phys_to_virt,
    .virt_to_phys = mock_virt_to_phys,
    .has_linear_physmap = mock_has_linear_physmap,
    .linear_physmap_base = NULL,
    .linear_physmap_limit = NULL,
};

const hal_translate_ops_t* hal_translate_ops(void) {
    return &mock_ops;
}

void verify_unmodified(size_t start, size_t end, uint8_t expected) {
    for (size_t i = start; i < end; i++) {
        assert(sim_memory[i] == expected);
    }
}

int main(void) {
    memset(sim_memory, 0xFF, SIM_MEMORY_SIZE);

    // 1. Zero size operation
    int rc = mm_memset_phys_range(SIM_BASE_ADDR, 0, 0);
    assert(rc == K_OK);
    verify_unmodified(0, SIM_MEMORY_SIZE, 0xFF);

    // 2. Invalid starting physical address
    rc = mm_memset_phys_range(0x90000000ULL, 0, 100);
    assert(rc == K_ERR_VM_UNMAPPED);
    verify_unmodified(0, SIM_MEMORY_SIZE, 0xFF);

    // 3. Physical address plus size overflow
    rc = mm_memset_phys_range((phys_addr_t)-50, 0, 100);
    assert(rc == K_ERR_INVALID_ARG);
    verify_unmodified(0, SIM_MEMORY_SIZE, 0xFF);

    // 4. Valid first page followed by an unmapped second page (crosses mapping boundary)
    // The simulated memory has 2 pages (8192 bytes). We try to clear 3 pages.
    rc = mm_memset_phys_range(SIM_BASE_ADDR + 4096, 0, 4096 * 2);
    assert(rc == K_ERR_VM_UNMAPPED);
    // Verify rejected operation did NOT modify the valid first page
    verify_unmodified(4096, SIM_MEMORY_SIZE, 0xFF);

    // 5. Valid zeroing within one mapped page (non-page-aligned)
    rc = mm_memset_phys_range(SIM_BASE_ADDR + 10, 0xA5, 20);
    assert(rc == K_OK);
    verify_unmodified(0, 10, 0xFF);
    for (size_t i = 10; i < 30; i++) assert(sim_memory[i] == 0xA5);
    verify_unmodified(30, SIM_MEMORY_SIZE, 0xFF);

    // 6. Valid zeroing across two mapped pages (crosses page boundary)
    // Clear back to FF first
    memset(sim_memory, 0xFF, SIM_MEMORY_SIZE);
    rc = mm_memset_phys_range(SIM_BASE_ADDR + 4090, 0, 10);
    assert(rc == K_OK);
    verify_unmodified(0, 4090, 0xFF);
    for (size_t i = 4090; i < 4100; i++) assert(sim_memory[i] == 0);
    verify_unmodified(4100, SIM_MEMORY_SIZE, 0xFF);

    // 7. Nonzero fill value test
    rc = mm_memset_phys_range(SIM_BASE_ADDR + 100, 0x42, 5);
    assert(rc == K_OK);
    for(size_t i=100; i < 105; i++) assert(sim_memory[i] == 0x42);

    printf("test_physmap_zero: PASS\n");
    return 0;
}
