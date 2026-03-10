#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../kernel/include/advanced/multikernel.h"
#include "../kernel/include/numa.h"
#include "../kernel/include/slab.h"

void sched_notify_ipc_ready(uint32_t core_id, uint32_t msg_type) {
    (void)core_id;
    (void)msg_type;
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
void mm_free_page(phys_addr_t page) {
    (void)page;
}
void hal_send_ipi_payload(uint32_t target_core, uint64_t payload) {
    (void)target_core;
    (void)payload;
}

int multicore_boot_secondary_cores(uint32_t core_count) {
    return (core_count >= 1U) ? 0 : -1;
}

int main(void) {
    assert(mk_boot_secondary_cores(2U) == URPC_SUCCESS);
    assert(mk_init_per_core_channels(2U, 4U) == URPC_SUCCESS);

    mk_channel_t c = {0};
    assert(mk_get_channel(0U, 1U, &c) == URPC_SUCCESS);
    assert(c.urpc_ring != NULL);

    urpc_msg_t msg = {.msg_type = 1U, .payload_size = 8U, .payload_data = {42U}};
    urpc_msg_t out = {0};

    // Fill, drain, and wrap around
    assert(mk_send_message(&c, 1U, msg.payload_data, 8U) == URPC_SUCCESS);
    assert(urpc_send(c.urpc_ring, &msg) == URPC_SUCCESS);
    assert(urpc_send(c.urpc_ring, &msg) == URPC_SUCCESS);
    assert(urpc_send(c.urpc_ring, &msg) == URPC_ERR_FULL);

    assert(urpc_receive(c.urpc_ring, &out) == URPC_SUCCESS);
    assert(urpc_receive(c.urpc_ring, &out) == URPC_SUCCESS);

    assert(mk_send_message(&c, 1U, msg.payload_data, 8U) == URPC_SUCCESS);
    assert(urpc_send(c.urpc_ring, &msg) == URPC_SUCCESS);

    int drained = 0;
    while (urpc_receive(c.urpc_ring, &out) == URPC_SUCCESS) {
        drained++;
    }
    assert(drained >= 3);

    // Message pool allocator
    mk_message_slot_t slots[2] = {0};
    mk_msg_pool_t pool = {0};
    assert(mk_msg_pool_init(&pool, slots, 2U) == URPC_SUCCESS);

    urpc_msg_t* a = mk_msg_alloc(&pool);
    urpc_msg_t* b = mk_msg_alloc(&pool);
    urpc_msg_t* cmsg = mk_msg_alloc(&pool);
    assert(a != NULL && b != NULL && cmsg == NULL);
    mk_msg_free(&pool, a);
    assert(mk_msg_alloc(&pool) != NULL);

    // NUMA descriptors
    assert(numa_discover_topology() == 0);
    assert(numa_set_node_descriptor(1U, 0x100000000ULL, 0x20000000ULL, 4U) == 0);
    numa_node_descriptor_t desc = {0};
    assert(numa_get_node_descriptor(1U, &desc) == 0);
    assert(desc.cpu_count == 4U);
    assert(numa_active_node_count() >= 2U);

    printf("Multikernel topology tests passed.\n");
    return 0;
}
