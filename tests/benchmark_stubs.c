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
