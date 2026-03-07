#include "../../include/io_subsys.h"
#include "../../include/mm.h"
#include "../../include/device.h"

/*
 * Implementation of Zero-Copy I/O Subsystem for OpenRAN User Plane.
 * Maps hardware queues directly to user-space, bypassing the kernel stack.
 */

int io_setup_zero_copy_nic_ring(uint32_t nic_device_id, zero_copy_nic_ring_t* out_ring, capability_t* cap) {
    if (!out_ring || !cap) return -1;

    // Verify capability to ensure only authorized RAN processes can access the NIC
    if ((cap->rights_mask & CAP_RIGHT_NETWORK_IO) == 0) {
        return -2; // Unauthorized access
    }

    device_mmio_window_t rx_window = {0};
    device_mmio_window_t tx_window = {0};

    if (device_lookup_mmio_window(DEVICE_CLASS_ETHERNET, nic_device_id, 0U, &rx_window) != 0) {
        return -3; // Unknown NIC RX window
    }

    if (device_lookup_mmio_window(DEVICE_CLASS_ETHERNET, nic_device_id, 1U, &tx_window) != 0) {
        return -3; // Unknown NIC TX window
    }

    if (vmm_map_device_mmio(rx_window.virt_base, rx_window.phys_base, cap, 0) != 0) {
        return -4;
    }

    if (vmm_map_device_mmio(tx_window.virt_base, tx_window.phys_base, cap, 0) != 0) {
        vmm_unmap_page(rx_window.virt_base);
        return -5;
    }

    // Populate the output structure for the User Plane
    out_ring->rx_ring_base = (void*)rx_window.virt_base;
    out_ring->tx_ring_base = (void*)tx_window.virt_base;
    out_ring->ring_size_bytes = rx_window.size_bytes;
    out_ring->msix_vector = rx_window.irq;

    return 0; // Success
}
