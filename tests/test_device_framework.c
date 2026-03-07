#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../kernel/include/device.h"
#include "../kernel/include/io_subsys.h"

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

int vmm_map_device_mmio(virt_addr_t vaddr, phys_addr_t paddr, capability_t* cap, int is_npu) {
    (void)vaddr;
    (void)paddr;
    (void)is_npu;
    return (cap != NULL) ? 0 : -1;
}

int vmm_unmap_page(virt_addr_t vaddr) {
    (void)vaddr;
    return 0;
}

int main(void) {
    assert(device_framework_init() == 0);
    assert(device_register_builtin_drivers() == 0);

    device_mmio_window_t rx = {0};
    device_mmio_window_t tx = {0};
    assert(device_lookup_mmio_window(DEVICE_CLASS_ETHERNET, 0U, 0U, &rx) == 0);
    assert(device_lookup_mmio_window(DEVICE_CLASS_ETHERNET, 0U, 1U, &tx) == 0);
    assert(rx.phys_base != 0U && tx.phys_base != 0U);

    capability_t cap = {
        .capability_id = 1U,
        .target_object_id = 1U,
        .rights_mask = CAP_RIGHT_NETWORK_IO,
    };
    zero_copy_nic_ring_t ring = {0};
    assert(io_setup_zero_copy_nic_ring(0U, &ring, &cap) == 0);
    assert(ring.rx_ring_base != NULL && ring.tx_ring_base != NULL);

    printf("Device framework tests passed.\n");
    return 0;
}
