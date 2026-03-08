#include "../kernel/include/advanced/ai_sched.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../kernel/include/mm.h"
#include "../kernel/include/advanced/formal_verif.h"

// Stubs for linking
phys_addr_t mm_alloc_page(uint32_t preferred_numa_node) {
    (void)preferred_numa_node;
    static uint8_t mem[4096];
    return (phys_addr_t)mem;
}
void mm_free_page(phys_addr_t page) { (void)page; }
int mm_pmm_init(void* memory_map, uint32_t map_size) { return 0; }
void mm_inc_page_ref(phys_addr_t page) { (void)page; }
void hal_tlb_flush(unsigned long long vaddr) { (void)vaddr; }
uint32_t hal_cpu_get_id(void) { return 0; }
void hal_cpu_halt(void) { }
void hal_send_ipi_payload(uint32_t target_core, uint64_t payload) { }


int main(void) {
    assert(vmm_init() == 0);

    capability_t valid_npu_cap = { .rights_mask = CAP_RIGHT_DEVICE_NPU };
    capability_t invalid_npu_cap = { .rights_mask = CAP_RIGHT_READ };

    // Valid mapping
    assert(vmm_map_device_mmio(0x3000U, 0x4000U, &valid_npu_cap, 1) == 0);

    // Invalid mapping should return -3 (ERR_CAP_DENIED)
    assert(vmm_map_device_mmio(0x5000U, 0x6000U, &invalid_npu_cap, 1) == -3);

    // Null capability should also be denied
    assert(vmm_map_device_mmio(0x7000U, 0x8000U, NULL, 1) == -3);

    printf("Capability policy tests passed.\n");
    return 0;
}

phys_addr_t hal_vmm_init_root(void) {
    return 0x1000U; /* return a valid dummy physical address */
}

int hal_vmm_map_page(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    (void)root_table;
    (void)vaddr;
    (void)paddr;
    (void)flags;
    return 0;
}

int hal_vmm_unmap_page(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t* unmapped_paddr) {
    (void)root_table;
    (void)vaddr;
    if (unmapped_paddr) *unmapped_paddr = 0x2000;
    return 0;
}

phys_addr_t hal_vmm_setup_address_space(phys_addr_t kernel_root_table) {
    (void)kernel_root_table;
    return 0x1000U;
}

int hal_vmm_get_mapping(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t* paddr, uint32_t* flags) {
    (void)root_table; (void)vaddr; (void)paddr; (void)flags;
    return -1;
}

int hal_vmm_update_mapping(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    (void)root_table; (void)vaddr; (void)paddr; (void)flags;
    return -1;
}

// Stubs for NUMA page migration dependencies
phys_addr_t pmm_alloc_pages_colored(int order, uint32_t preferred_numa_node, uint32_t flags, mm_color_config_t *color_config) {
    (void)order; (void)preferred_numa_node; (void)flags; (void)color_config;
    return 0;
}
phys_addr_t mm_alloc_pages_order(int order, uint32_t preferred_numa_node, uint32_t flags) {
    (void)order; (void)preferred_numa_node; (void)flags;
    return 0;
}
// Mocks
#include "../kernel/include/slab.h"
#include <stdlib.h>

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
}
void* kmalloc(size_t size) {
    return malloc(size);
}
void kfree(void* ptr) {
    // DO NOTHING
}

void ai_sched_collect_sample(ai_sched_context_t* ctx,
                             uint64_t time_slice_ms,
                             uint64_t cpu_time_consumed,
                             uint32_t run_queue_depth,
                             uint32_t context_switches) {}
