#ifndef BHARAT_VIRTIO_PCI_H
#define BHARAT_VIRTIO_PCI_H

#include "device/pci.h"
#include "virtqueue.h"
#include <stdint.h>
#include <stdbool.h>

#define VIRTIO_PCI_CAP_COMMON_CFG  1
#define VIRTIO_PCI_CAP_NOTIFY_CFG  2
#define VIRTIO_PCI_CAP_ISR_CFG     3
#define VIRTIO_PCI_CAP_DEVICE_CFG  4

typedef struct {
    pci_device_t *pci_dev;

    // Registers / Structures MMIO mapped
    volatile struct virtio_pci_common_cfg *common_cfg;
    volatile uint8_t *isr_cfg;
    volatile uint8_t *notify_base;
    uint32_t notify_mult;
    volatile uint8_t *device_cfg;

    // Feature negotiation
    uint64_t device_features;
    uint64_t driver_features;
} bh_virtio_pci_device_t;

/**
 * Probes and maps a VirtIO modern PCI device.
 */
int bh_virtio_pci_probe(bh_virtio_pci_device_t *vdev, pci_device_t *pci_dev);

/**
 * Negotiates device features with the VirtIO device.
 */
int bh_virtio_pci_negotiate_features(bh_virtio_pci_device_t *vdev, uint64_t features, uint64_t *out_features);

/**
 * Configures and registers a virtqueue for the device.
 */
int bh_virtio_pci_setup_queue(bh_virtio_pci_device_t *vdev,
                              uint16_t queue_idx,
                              bh_virtqueue_t *vq,
                              bh_virtq_desc_t *desc_table,
                              bh_virtq_avail_t *avail_ring,
                              bh_virtq_used_t *used_ring);

/**
 * Starts the device (sets DRIVER_OK status).
 */
int bh_virtio_pci_start_device(bh_virtio_pci_device_t *vdev);

/**
 * Stops/resets the device status.
 */
void bh_virtio_pci_reset_device(bh_virtio_pci_device_t *vdev);

/**
 * Triggers a notification to the host for the given virtqueue.
 */
void bh_virtio_pci_notify_queue(bh_virtio_pci_device_t *vdev, uint16_t queue_idx, bh_virtqueue_t *vq);

#endif // BHARAT_VIRTIO_PCI_H
