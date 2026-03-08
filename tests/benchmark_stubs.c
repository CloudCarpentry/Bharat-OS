#include "../kernel/include/sched.h"
#include "../kernel/include/slab.h"
#include <stdlib.h>
#include <string.h>

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
    (void)cache;
    (void)obj;
}

uint32_t hal_cpu_get_id(void) {
    return 0;
}

void hal_cpu_halt(void) {
}

void hal_vmm_switch_address_space(address_space_t* as) {
    (void)as;
}

void hal_fpu_disable(void) {
}

void hal_fpu_enable(void) {
}

void hal_fpu_save_state(void* kthread) {
    (void)kthread;
}

void hal_fpu_restore_state(void* kthread) {
    (void)kthread;
}

void hal_fpu_init_thread_state(void* kthread) {
    (void)kthread;
}

phys_addr_t hal_vmm_init_root(void) {
    return 0x1000U;
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
    if (unmapped_paddr) *unmapped_paddr = 0x2000U;
    return 0;
}

phys_addr_t hal_vmm_setup_address_space(phys_addr_t kernel_root_table) {
    (void)kernel_root_table;
    return 0x3000U;
}

int hal_vmm_get_mapping(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t* paddr, uint32_t* flags) {
    (void)root_table;
    (void)vaddr;
    if (paddr) *paddr = 0x4000U;
    if (flags) *flags = 0;
    return 0;
}

int hal_vmm_update_mapping(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    (void)root_table;
    (void)vaddr;
    (void)paddr;
    (void)flags;
    return 0;
}
