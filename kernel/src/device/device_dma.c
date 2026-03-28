#include "device.h"
#include "mm/dma.h"
#include "mm/iommu.h"
#include "slab.h"
#include "console/console_core.h"

// Ensure the full types are available
#include "mm/dma.h"

// In a real framework, these would be attached to the actual `device_t` struct
// For this standalone feature, we will use a linked list mapping dev pointers to their caps.
typedef struct dev_dma_node {
    device_t *dev;
    dma_caps_t dma_caps;
    iommu_dev_info_t iommu_info;
    dev_dma_mode_t effective_mode;
    struct dev_dma_node *next;
} dev_dma_node_t;

static dev_dma_node_t *g_dev_dma_list = NULL;
#define DEV_DMA_NODE_CAPACITY 64U
static dev_dma_node_t g_dev_dma_nodes[DEV_DMA_NODE_CAPACITY];
// Missing spinlock for simplicity, would be added in prod.

static dev_dma_node_t *alloc_node(void) {
    for (size_t i = 0; i < DEV_DMA_NODE_CAPACITY; ++i) {
        if (g_dev_dma_nodes[i].dev == NULL) {
            return &g_dev_dma_nodes[i];
        }
    }
    return NULL;
}

static dev_dma_node_t *find_or_create_node(device_t *dev) {
    dev_dma_node_t *curr = g_dev_dma_list;
    while (curr) {
        if (curr->dev == dev) return curr;
        curr = curr->next;
    }

    dev_dma_node_t *node = alloc_node();
    if (node) {
        node->dev = dev;
        // defaults
        node->dma_caps.dma_mask = 0xFFFFFFFF; // 32-bit default
        node->dma_caps.max_segment_size = 65536;
        node->dma_caps.alignment = 1;
        node->dma_caps.coherent = false;
        node->dma_caps.require_isolation = false;
        node->dma_caps.allow_bypass = true;
        node->dma_caps.allow_identity = true;
        node->dma_caps.require_contiguous = false;

        node->iommu_info.present_on_iommu_path = false;
        node->iommu_info.can_be_managed = false;
        node->iommu_info.default_bypass = true;
        node->iommu_info.force_identity = false;
        node->iommu_info.requester_id = 0;
        node->iommu_info.stream_id = 0;
        node->iommu_info.sid_mask = 0;

        node->effective_mode = DEV_DMA_MODE_UNKNOWN;

        node->next = g_dev_dma_list;
        g_dev_dma_list = node;
    }
    return node;
}

int device_set_dma_caps(device_t *dev, const dma_caps_t *caps) {
    if (!dev || !caps) return -1;
    dev_dma_node_t *node = find_or_create_node(dev);
    if (!node) return -2;
    node->dma_caps = *caps;
    return 0;
}

int device_set_iommu_info(device_t *dev, const iommu_dev_info_t *info) {
    if (!dev || !info) return -1;
    dev_dma_node_t *node = find_or_create_node(dev);
    if (!node) return -2;
    node->iommu_info = *info;
    return 0;
}

const dma_caps_t *device_get_dma_caps(const device_t *dev) {
    if (!dev) return NULL;
    dev_dma_node_t *node = find_or_create_node((device_t*)dev);
    return node ? &node->dma_caps : NULL;
}

const iommu_dev_info_t *device_get_iommu_info(const device_t *dev) {
    if (!dev) return NULL;
    dev_dma_node_t *node = find_or_create_node((device_t*)dev);
    return node ? &node->iommu_info : NULL;
}

int device_resolve_dma_iommu_policy(device_t *dev,
                                    dma_caps_t *dma_out,
                                    iommu_dev_info_t *iommu_out) {
    if (!dev) return -1;

    dev_dma_node_t *node = find_or_create_node(dev);
    if (!node) return -2;

    if (dma_out) *dma_out = node->dma_caps;
    if (iommu_out) *iommu_out = node->iommu_info;

    // Resolve mode based on capabilities and global IOMMU presence
    if (!iommu_available() || !node->iommu_info.present_on_iommu_path) {
        if (node->dma_caps.require_isolation) {
            node->effective_mode = DEV_DMA_MODE_BLOCKED;
        } else {
            node->effective_mode = DEV_DMA_MODE_DIRECT; // Fallback to direct. Bounce might be chosen dynamically at map time based on mask
        }
    } else {
        // On IOMMU path
        if (node->iommu_info.force_identity) {
            node->effective_mode = DEV_DMA_MODE_IDENTITY;
        } else if (node->iommu_info.can_be_managed) {
            node->effective_mode = DEV_DMA_MODE_IOMMU;
        } else if (node->iommu_info.default_bypass) {
            node->effective_mode = DEV_DMA_MODE_DIRECT;
        } else {
            node->effective_mode = DEV_DMA_MODE_BLOCKED;
        }
    }

    return 0;
}

dev_dma_mode_t device_get_effective_dma_mode(device_t *dev) {
    if (!dev) return DEV_DMA_MODE_UNKNOWN;
    dev_dma_node_t *node = find_or_create_node(dev);
    if (!node) return DEV_DMA_MODE_UNKNOWN;

    if (node->effective_mode == DEV_DMA_MODE_UNKNOWN) {
        device_resolve_dma_iommu_policy(dev, NULL, NULL);
    }

    return node->effective_mode;
}

void device_dump_dma_caps(const device_t *dev) {
    if (!dev) return;
    dev_dma_node_t *node = find_or_create_node((device_t*)dev);
    if (!node) return;

    console_write_raw("Device DMA Caps Dump:\n", 22);

    if (node->dma_caps.coherent) {
        console_write_raw("  Coherent: YES\n", 16);
    } else {
        console_write_raw("  Coherent: NO\n", 15);
    }

    if (node->effective_mode == DEV_DMA_MODE_IOMMU) {
        console_write_raw("  Mode: IOMMU\n", 14);
    } else if (node->effective_mode == DEV_DMA_MODE_DIRECT) {
        console_write_raw("  Mode: DIRECT\n", 15);
    } else if (node->effective_mode == DEV_DMA_MODE_BOUNCE) {
        console_write_raw("  Mode: BOUNCE\n", 15);
    } else if (node->effective_mode == DEV_DMA_MODE_BLOCKED) {
        console_write_raw("  Mode: BLOCKED\n", 16);
    }
}

// --- Boot test to verify resolution functionality ---
void test_device_dma_dump(void) {
    // Fake devices for tests by passing arbitrary pointers for identification
    device_t *dummy_pci_dev = (device_t*)0x1000;
    device_t *dummy_platform_dev = (device_t*)0x2000;

    // Register PCI dev
    dma_caps_t pci_dma = {
        .dma_mask = 0xFFFFFFFF,
        .max_segment_size = 65536,
        .alignment = 1,
        .coherent = true,
        .require_isolation = false,
        .allow_bypass = true,
        .allow_identity = true,
        .require_contiguous = false
    };
    iommu_dev_info_t pci_iommu = {
        .present_on_iommu_path = true,
        .can_be_managed = true,
        .default_bypass = false,
        .force_identity = false,
        .requester_id = 0x0100,
        .stream_id = 0,
        .sid_mask = 0
    };

    device_set_dma_caps(dummy_pci_dev, &pci_dma);
    device_set_iommu_info(dummy_pci_dev, &pci_iommu);
    device_resolve_dma_iommu_policy(dummy_pci_dev, NULL, NULL);

    console_write_raw("PCI Dev Test:\n", 14);
    device_dump_dma_caps(dummy_pci_dev);

    // Register Platform Dev
    dma_caps_t plat_dma = {
        .dma_mask = 0xFFFFFFFF,
        .max_segment_size = 65536,
        .alignment = 1,
        .coherent = false,
        .require_isolation = true,
        .allow_bypass = false,
        .allow_identity = false,
        .require_contiguous = true
    };
    iommu_dev_info_t plat_iommu = {
        .present_on_iommu_path = false, // Platform device doesn't have IOMMU
        .can_be_managed = false,
        .default_bypass = true,
        .force_identity = false,
        .requester_id = 0,
        .stream_id = 0,
        .sid_mask = 0
    };

    device_set_dma_caps(dummy_platform_dev, &plat_dma);
    device_set_iommu_info(dummy_platform_dev, &plat_iommu);
    device_resolve_dma_iommu_policy(dummy_platform_dev, NULL, NULL);

    console_write_raw("Platform Dev Test (Isolation Req, No IOMMU):\n", 45);
    device_dump_dma_caps(dummy_platform_dev);
}
