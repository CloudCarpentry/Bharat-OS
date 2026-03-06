#ifndef BHARAT_VIRTIO_H
#define BHARAT_VIRTIO_H

#include <stdint.h>

/*
 * Bharat-OS Paravirtualized VirtIO Guest Drivers
 * Allows Bharat-OS to run at blazing-fast speeds natively as a guest VM inside
 * Data Center hypervisors like QEMU/KVM, Hyper-V, and VMware.
 */

// VirtIO Device Types
#define VIRTIO_DEV_NETWORK  1
#define VIRTIO_DEV_BLOCK    2
#define VIRTIO_DEV_CONSOLE  3
#define VIRTIO_DEV_ENTROPY  4
#define VIRTIO_DEV_GPU     16

// Standard Virtqueue structure for asynchronous guest/host communication
typedef struct {
    uint32_t queue_size;
    void* desc_table_phys;
    void* avail_ring_phys;
    void* used_ring_phys;
    
    uint16_t last_used_idx;
} virtqueue_t;

// Generic VirtIO Device handle
typedef struct {
    uint32_t pci_bus;
    uint32_t pci_device;
    uint32_t pci_function;
    
    uint32_t device_type;
    uint32_t features; // Negotiated features with Hypervisor
    
    void* io_base;
    
    // Active virtqueues (e.g., rx/tx for net, req/resp for block)
    virtqueue_t* vqs;
    uint32_t num_vqs;
} virtio_device_t;

// Discover VirtIO devices on the PCI bus
int virtio_probe_devices(virtio_device_t* out_devices, uint32_t max_devs);

// Initialize a specific VirtIO device and negotiate features with the Hypervisor
int virtio_init_device(virtio_device_t* dev, uint32_t requested_features);

#endif // BHARAT_VIRTIO_H
