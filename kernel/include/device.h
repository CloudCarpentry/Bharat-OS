#ifndef BHARAT_DEVICE_H
#define BHARAT_DEVICE_H

#include <stdint.h>

#include "mm.h"

typedef enum {
    DEVICE_CLASS_UART = 0,
    DEVICE_CLASS_SPI,
    DEVICE_CLASS_I2C,
    DEVICE_CLASS_SDMMC,
    DEVICE_CLASS_ETHERNET,
    DEVICE_CLASS_GPIO,
    DEVICE_CLASS_USB,
    DEVICE_CLASS_CAN,
    DEVICE_CLASS_VIRTIO,
    DEVICE_CLASS_PLATFORM,
} device_class_t;

typedef enum {
    DEVICE_BUS_PCI = 0,
    DEVICE_BUS_PLATFORM_MMIO,
    DEVICE_BUS_USB,
    DEVICE_BUS_I2C,
    DEVICE_BUS_SPI,
    DEVICE_BUS_UART,
    DEVICE_BUS_GPIO,
    DEVICE_BUS_SDMMC,
    DEVICE_BUS_CAN,
    DEVICE_BUS_VIRTIO,
} device_bus_t;

typedef enum {
    DEVICE_POWER_D0 = 0,
    DEVICE_POWER_D1,
    DEVICE_POWER_D2,
    DEVICE_POWER_D3,
} device_power_state_t;

enum {
    DEVICE_SECURITY_CAPABILITY_GATED = (1U << 0),
    DEVICE_SECURITY_IOMMU_DMA_GUARD = (1U << 1),
    DEVICE_SECURITY_SIGNED_FW_ONLY   = (1U << 2),
};

enum {
    DEVICE_PERF_IRQ_AFFINITY      = (1U << 0),
    DEVICE_PERF_ZERO_COPY_RXTX    = (1U << 1),
    DEVICE_PERF_NAPI_BUDGETED_RX  = (1U << 2),
};

enum {
    DEVICE_HW_FEAT_MSI_X         = (1U << 0),
    DEVICE_HW_FEAT_RX_CSUM       = (1U << 1),
    DEVICE_HW_FEAT_TX_CSUM       = (1U << 2),
    DEVICE_HW_FEAT_TSO           = (1U << 3),
    DEVICE_HW_FEAT_RSS           = (1U << 4),
    DEVICE_HW_FEAT_CAN_FD        = (1U << 5),
    DEVICE_HW_FEAT_VIRTIO_MODERN = (1U << 6),
};

typedef struct {
    device_class_t class_id;
    uint32_t device_id;
    uint32_t window_id;
    phys_addr_t phys_base;
    virt_addr_t virt_base;
    uint32_t size_bytes;
    uint32_t irq;
    uint8_t in_use;
} device_mmio_window_t;

typedef struct {
    device_bus_t bus;
    device_class_t class_id;
    uint32_t device_id;
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t class_code;
    uint8_t subclass_code;
    const char* compatible;
    uint32_t irq;
    uint8_t hotpluggable;
    device_power_state_t power_state;
    uint8_t rx_queue_count;
    uint8_t tx_queue_count;
    uint32_t security_flags;
    uint32_t perf_flags;
    uint32_t hw_feature_flags;
    uint8_t in_use;
} device_desc_t;

typedef struct {
    device_bus_t bus;
    device_class_t class_id;
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t class_code;
    uint8_t subclass_code;
    const char* compatible;
} device_match_t;

typedef struct {
    const char* name;
    device_class_t class_id;
    device_bus_t bus;
    uint32_t device_id;

    /* Legacy probe callback for older stubs. */
    int (*probe)(void);

    /* Preferred callbacks for bus/device-aware model. */
    int (*probe_device)(const device_desc_t* dev, void** out_ctx);
    int (*remove_device)(void* ctx);
    int (*suspend)(void* ctx, device_power_state_t target_state);
    int (*resume)(void* ctx);
    void (*irq_handler)(uint32_t irq, void* ctx);

    void* ctx;
    device_match_t match;
} device_driver_t;

int device_framework_init(void);
int device_register_driver(const device_driver_t* driver);
int device_register_mmio_window(const device_mmio_window_t* window);
int device_lookup_mmio_window(device_class_t class_id,
                              uint32_t device_id,
                              uint32_t window_id,
                              device_mmio_window_t* out_window);
int device_dispatch_irq(uint32_t irq);

int device_register_bus_device(const device_desc_t* dev);
int device_hotplug_add(const device_desc_t* dev);
int device_hotplug_remove(device_bus_t bus, uint32_t device_id);
int device_bind_drivers(void);
int device_set_power_state(device_bus_t bus, uint32_t device_id, device_power_state_t target_state);

int device_register_builtin_drivers(void);

int pci_discover_nic(device_mmio_window_t* rx_window, device_mmio_window_t* tx_window);

#endif // BHARAT_DEVICE_H
