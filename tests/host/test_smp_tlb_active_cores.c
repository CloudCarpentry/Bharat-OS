#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define TESTING 1

#include "../../kernel/include/hal/hal.h"
#include "../../kernel/include/sched/sched.h"
#include "../../kernel/include/bharat/cpu_local.h"
#include "../../kernel/include/slab.h"
#include "../../kernel/include/hal/hal_discovery.h"
#include "../../kernel/include/mm/aspace.h"
#include "../../kernel/include/mm/mm_remote.h"

cpu_local_t g_cpu_locals[MAX_CPUS];
system_discovery_t g_discovery;

// --- Mocking ---

void* kmalloc(size_t size) {
    return malloc(size);
}

void kfree(void* ptr) {
    free(ptr);
}

void kernel_panic(const char* msg) {
    printf("KERNEL PANIC: %s\n", msg);
    exit(1);
}

system_discovery_t* hal_get_system_discovery(void) {
    return &g_discovery;
}

// For sched tests we need these
bh_process_t* process_create(const char* name) {
    // Stub
    bh_process_t* p = kmalloc(sizeof(bh_process_t));
    memset(p, 0, sizeof(*p));
    return p;
}

void hal_ipi_send(uint32_t target_core, uint32_t vector) {
    // Stub
}

// Tests
void test_smp_tlb_active_cores(void) {
    printf("Running test_smp_tlb_active_cores...\n");

    // Reset test environment
    memset(&g_discovery, 0, sizeof(g_discovery));
    memset(&g_cpu_locals, 0, sizeof(g_cpu_locals));

    g_discovery.topology.cpu_count = 2; // Only 2 cores discovered

    // We will set up g_cpu_locals for the 2 online cores
    g_cpu_locals[0].is_online = true;
    g_cpu_locals[1].is_online = true;
    // Core 2 is offline
    g_cpu_locals[2].is_online = false;

    // Simulate an address space
    address_space_t test_as;
    memset(&test_as, 0, sizeof(test_as));
    test_as.object_id = 1;
    test_as.tlb_gen = 0;
    test_as.active_mask = (1ULL << 0) | (1ULL << 1); // Only core 0 and 1 are active in this aspace

    // Now, let's artificially set the current_as for an *offline* core (e.g. core 2) to a junk pointer
    // to prove that the tlb shootdown won't crash trying to access it
    g_cpu_locals[2].current_as = (address_space_t*)0xDEADBEEF; // Junk pointer
    g_cpu_locals[2].current_as_id = 1;

    // Core 1 is online and has the address space
    g_cpu_locals[1].current_as = &test_as;
    g_cpu_locals[1].current_as_id = 1;

    // Call vmm_send_tlb_invalidate from core 0
    // Because core 2 is offline, it should be ignored and not crash.
    extern void vmm_send_tlb_invalidate(uint64_t aspace_id, uint64_t va, uint64_t len, uint32_t type);

    // We expect this NOT to crash.
    vmm_send_tlb_invalidate(1, 0x1000, 0x1000, 0);

    printf("test_smp_tlb_active_cores passed.\n");
}

int main() {
    test_smp_tlb_active_cores();
    printf("All host TLB SMP tests passed!\n");
    return 0;
}
