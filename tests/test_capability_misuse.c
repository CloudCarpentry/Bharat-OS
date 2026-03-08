#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../kernel/include/capability.h"

static address_space_t g_as = { .root_table = 0x1000U };

address_space_t* mm_create_address_space(void) {
    return &g_as;
}

phys_addr_t mm_alloc_page(uint32_t preferred_numa_node) {
    (void)preferred_numa_node;
    return 0;
}

void mm_free_page(phys_addr_t page) {
    (void)page;
}





int main(void) {
    sched_init();
    kprocess_t* p = process_create("cap-misuse");
    assert(p != NULL);

    capability_table_t* t = (capability_table_t*)p->security_sandbox_ctx;
    assert(t != NULL);

    uint32_t cap = 0;

    // Invalid rights for endpoint object should fail.
    assert(cap_table_grant(t, CAP_OBJ_ENDPOINT, 1U, CAP_PERM_MAP, &cap) != 0);

    // Valid grant succeeds.
    assert(cap_table_grant(t, CAP_OBJ_ENDPOINT, 1U, CAP_PERM_SEND | CAP_PERM_DELEGATE, &cap) == 0);

    // Delegating unsupported rights should fail.
    uint32_t delegated = 0;
    assert(cap_table_delegate(t, t, cap, CAP_PERM_MAP, &delegated) != 0);

    printf("Capability misuse tests passed.\n");
    return 0;
}





#include "../kernel/include/slab.h"
#include <stdlib.h>
#include <string.h>

// Some tests were crashing on free() because thread_destroy called kcache_free on pointers that weren't allocated by kcache_alloc (like stack/global variables in tests).
// Since these are stub tests without full memory management setup, let's just make kcache_free a no-op for tests.
kcache_t* kcache_create(const char* name, size_t size) {
    kcache_t* c = malloc(sizeof(kcache_t));
    if(c) {
        c->object_size = size;
        c->name = name;
    }
    return c;
}
void* kcache_alloc(kcache_t* cache) {
    if(!cache) return NULL;
    return malloc(cache->object_size);
}
void kcache_free(kcache_t* cache, void* obj) {
    // DO NOTHING in tests to avoid free() errors on statically allocated mock threads.
}

uint32_t hal_cpu_get_id(void) { return 0; }
void hal_cpu_halt(void) { }

// Stubs for NUMA page migration dependencies
phys_addr_t pmm_alloc_pages_colored(int order, uint32_t preferred_numa_node, uint32_t flags, mm_color_config_t *color_config) {
    (void)order; (void)preferred_numa_node; (void)flags; (void)color_config;
    return 0;
}
phys_addr_t mm_alloc_pages_order(int order, uint32_t preferred_numa_node, uint32_t flags) {
    (void)order; (void)preferred_numa_node; (void)flags;
    return 0;
}
int hal_vmm_get_mapping(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t* paddr, uint32_t* flags) {
    (void)root_table; (void)vaddr; (void)paddr; (void)flags;
    return -1;
}
int hal_vmm_update_mapping(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    (void)root_table; (void)vaddr; (void)paddr; (void)flags;
    return -1;
}
void tlb_shootdown(virt_addr_t vaddr) {
    (void)vaddr;
}
