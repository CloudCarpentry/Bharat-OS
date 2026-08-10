#include "virtio_pci.h"
#include "device/device_mmio.h"

#define VIRTIO_STATUS_RESET         0
#define VIRTIO_STATUS_ACKNOWLEDGE   1
#define VIRTIO_STATUS_DRIVER        2
#define VIRTIO_STATUS_FEATURES_OK   8
#define VIRTIO_STATUS_DRIVER_OK     4
#define VIRTIO_STATUS_FAILED        128

struct virtio_pci_common_cfg {
    uint32_t device_feature_select;
    uint32_t device_feature;
    uint32_t driver_feature_select;
    uint32_t driver_feature;
    uint16_t config_msix_vector;
    uint16_t num_queues;
    uint8_t  device_status;
    uint8_t  config_generation;

    uint16_t queue_select;
    uint16_t queue_size;
    uint16_t queue_msix_vector;
    uint16_t queue_enable;
    uint16_t queue_notify_off;
    uint64_t queue_desc;
    uint64_t queue_avail;
    uint64_t queue_used;
};

int bh_virtio_pci_probe(bh_virtio_pci_device_t *vdev, pci_device_t *pci_dev) {
    if (!vdev || !pci_dev) return -1;

    __builtin_memset(vdev, 0, sizeof(*vdev));
    vdev->pci_dev = pci_dev;

    // Enable bus master and memory mapping
    pci_enable_device(pci_dev);

    // Parse capabilities
    uint8_t cap_ptr = pci_config_read8(pci_dev, 0x34);
    while (cap_ptr != 0 && cap_ptr < 0xFC) {
        uint8_t cap_id = pci_config_read8(pci_dev, cap_ptr);
        if (cap_id == 0x09) { // Vendor Specific
            uint8_t cfg_type = pci_config_read8(pci_dev, cap_ptr + 3);
            uint8_t bar = pci_config_read8(pci_dev, cap_ptr + 4);
            uint32_t offset = pci_config_read32(pci_dev, cap_ptr + 8);
            uint32_t length = pci_config_read32(pci_dev, cap_ptr + 12);

            if (bar < 6) {
                uint64_t bar_phys = pci_dev->bar[bar] & ~0xFU;
                uint64_t phys = bar_phys + offset;
                void *virt = NULL;
                if (bh_device_mmio_map(phys, length, 0, &virt) == 0) {
                    switch (cfg_type) {
                        case VIRTIO_PCI_CAP_COMMON_CFG:
                            vdev->common_cfg = (volatile struct virtio_pci_common_cfg *)virt;
                            break;
                        case VIRTIO_PCI_CAP_ISR_CFG:
                            vdev->isr_cfg = (volatile uint8_t *)virt;
                            break;
                        case VIRTIO_PCI_CAP_DEVICE_CFG:
                            vdev->device_cfg = (volatile uint8_t *)virt;
                            break;
                        case VIRTIO_PCI_CAP_NOTIFY_CFG:
                            vdev->notify_base = (volatile uint8_t *)virt;
                            vdev->notify_mult = pci_config_read32(pci_dev, cap_ptr + 16);
                            break;
                    }
                }
            }
        }
        cap_ptr = pci_config_read8(pci_dev, cap_ptr + 1);
    }

    if (!vdev->common_cfg || !vdev->notify_base) {
        return -2; // Required capabilities missing or failed to map
    }

    return 0;
}

int bh_virtio_pci_negotiate_features(bh_virtio_pci_device_t *vdev, uint64_t features, uint64_t *out_features) {
    if (!vdev || !vdev->common_cfg) return -1;

    // Read device features
    uint64_t dev_features = 0;
    vdev->common_cfg->device_feature_select = 0;
    dev_features |= vdev->common_cfg->device_feature;
    vdev->common_cfg->device_feature_select = 1;
    dev_features |= ((uint64_t)vdev->common_cfg->device_feature) << 32;

    vdev->device_features = dev_features;

    // Intersect and write back
    uint64_t agreed = dev_features & features;
    vdev->common_cfg->driver_feature_select = 0;
    vdev->common_cfg->driver_feature = (uint32_t)agreed;
    vdev->common_cfg->driver_feature_select = 1;
    vdev->common_cfg->driver_feature = (uint32_t)(agreed >> 32);

    vdev->driver_features = agreed;

    if (out_features) {
        *out_features = agreed;
    }

    return 0;
}

int bh_virtio_pci_setup_queue(bh_virtio_pci_device_t *vdev,
                              uint16_t queue_idx,
                              bh_virtqueue_t *vq,
                              bh_virtq_desc_t *desc_table,
                              bh_virtq_avail_t *avail_ring,
                              bh_virtq_used_t *used_ring) {
    if (!vdev || !vdev->common_cfg || !vq) return -1;

    volatile struct virtio_pci_common_cfg *cfg = vdev->common_cfg;

    // 1. Select the queue
    cfg->queue_select = queue_idx;

    // 2. Query queue size from device
    uint16_t q_size = cfg->queue_size;
    if (q_size == 0) return -2; // Queue not supported

    // Align queue size to what we have or device limit
    if (q_size > VIRTIO_RING_SIZE) {
        q_size = VIRTIO_RING_SIZE;
    }
    cfg->queue_size = q_size;

    // 3. Initialize software representation
    bh_virtqueue_init(vq, q_size, desc_table, avail_ring, used_ring);

    // 4. Write physical addresses to configuration registers
    extern uint64_t g_kernel_virt_offset;
    uint64_t desc_phys = (uint64_t)(uintptr_t)desc_table - g_kernel_virt_offset;
    uint64_t avail_phys = (uint64_t)(uintptr_t)avail_ring - g_kernel_virt_offset;
    uint64_t used_phys = (uint64_t)(uintptr_t)used_ring - g_kernel_virt_offset;

    cfg->queue_desc = desc_phys;
    cfg->queue_avail = avail_phys;
    cfg->queue_used = used_phys;

    // 5. Enable the queue
    cfg->queue_enable = 1;

    return 0;
}

int bh_virtio_pci_start_device(bh_virtio_pci_device_t *vdev) {
    if (!vdev || !vdev->common_cfg) return -1;

    volatile struct virtio_pci_common_cfg *cfg = vdev->common_cfg;

    // Set Acknowledge, Driver, and Features OK status
    cfg->device_status = VIRTIO_STATUS_ACKNOWLEDGE;
    cfg->device_status |= VIRTIO_STATUS_DRIVER;
    cfg->device_status |= VIRTIO_STATUS_FEATURES_OK;

    // Check if device accepted features
    if ((cfg->device_status & VIRTIO_STATUS_FEATURES_OK) == 0) {
        cfg->device_status |= VIRTIO_STATUS_FAILED;
        return -2;
    }

    cfg->device_status |= VIRTIO_STATUS_DRIVER_OK;
    return 0;
}

void bh_virtio_pci_reset_device(bh_virtio_pci_device_t *vdev) {
    if (!vdev || !vdev->common_cfg) return;
    vdev->common_cfg->device_status = VIRTIO_STATUS_RESET;
    // Memory barrier to ensure reset takes effect
    __asm__ volatile("" ::: "memory");
}

void bh_virtio_pci_notify_queue(bh_virtio_pci_device_t *vdev, uint16_t queue_idx, bh_virtqueue_t *vq) {
    if (!vdev || !vdev->common_cfg || !vq) return;

    vdev->common_cfg->queue_select = queue_idx;
    uint16_t notify_off = vdev->common_cfg->queue_notify_off;

    volatile uint16_t *notify_addr = (volatile uint16_t *)(vdev->notify_base + notify_off * vdev->notify_mult);
    __asm__ volatile("" ::: "memory");
    *notify_addr = queue_idx;
    __asm__ volatile("" ::: "memory");
}
