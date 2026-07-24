#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <mm/aspace.h>
#include <mm/tlb.h>
#include <kernel/status.h>

// Mock active_core_count
uint32_t g_active_core_count = 4;

// Mocks and stubs
void kernel_panic(const char* msg) {
    printf("PANIC: %s\n", msg);
}

void console_log(int level, const char* fmt, ...) {
    (void)level; (void)fmt;
}

// Minimal tlb_pending mock/interface
#include "tlb_pending.h"

// Global mock state
uint32_t mock_cpu_id = 0;
uint32_t hal_cpu_get_id(void) { return mock_cpu_id; }

// Dummy aspace functions to avoid linking aspace.c
int aspace_region_reserve(address_space_t *aspace, uintptr_t base, size_t length, uint32_t prot, uint32_t map_flags, vm_inherit_t inherit, vm_region_t **out_region) {
    if (aspace->state == ASPACE_STATE_POISONED) return K_ERR_BAD_STATE;
    return K_OK;
}
void aspace_mark_poisoned(address_space_t *aspace) {
    aspace->state = ASPACE_STATE_POISONED;
}

void test_tlb_pending_topology(void) {
    printf("Running test_tlb_pending_topology...\n");
    tlb_pending_init();

    uint32_t reqid;
    mock_cpu_id = 0;

    assert(tlb_pending_alloc(123, 0x2, &reqid) >= 0); // Core 1

    // Invalid: target includes core 4 (outside g_active_core_count=4)
    assert(tlb_pending_alloc(123, 0x10, &reqid) < 0);

    // Invalid: target includes self (core 0)
    assert(tlb_pending_alloc(123, 0x1, &reqid) < 0);

    printf("test_tlb_pending_topology passed!\n");
}

void test_aspace_poisoning(void) {
    printf("Running test_aspace_poisoning...\n");
    address_space_t aspace;
    memset(&aspace, 0, sizeof(aspace));
    aspace.state = ASPACE_STATE_ACTIVE;

    aspace_mark_poisoned(&aspace);
    assert(aspace.state == ASPACE_STATE_POISONED);

    // Check that operations fail
    assert(aspace_region_reserve(&aspace, 0x1000, 0x1000, 0, 0, 0, NULL) == K_ERR_BAD_STATE);

    printf("test_aspace_poisoning passed!\n");
}

int main(void) {
    test_tlb_pending_topology();
    test_aspace_poisoning();
    return 0;
}
