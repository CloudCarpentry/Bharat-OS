#include "device.h"

#include <stddef.h>

static int driver_probe_noop(void) {
    return 0;
}

static int driver_probe_device_noop(const device_desc_t* dev, void** out_ctx) {
    (void)dev;
    if (out_ctx) {
        *out_ctx = NULL;
    }
    return 0;
}

static int driver_remove_noop(void* ctx) {
    (void)ctx;
    return 0;
}

static int driver_suspend_noop(void* ctx, device_power_state_t target_state) {
    (void)ctx;
    (void)target_state;
    return 0;
}

static int driver_resume_noop(void* ctx) {
    (void)ctx;
    return 0;
}

static void driver_irq_noop(uint32_t irq, void* ctx) {
    (void)irq;
    (void)ctx;
}

int device_register_builtin_drivers(void) {
    static const device_driver_t drivers[] = {
        {.name = "uart0", .class_id = DEVICE_CLASS_UART, .bus = DEVICE_BUS_UART, .device_id = 0U,
         .probe = driver_probe_noop, .probe_device = driver_probe_device_noop,
         .remove_device = driver_remove_noop, .suspend = driver_suspend_noop, .resume = driver_resume_noop,
         .irq_handler = driver_irq_noop, .ctx = NULL,
         .match = {.bus = DEVICE_BUS_UART, .class_id = DEVICE_CLASS_UART, .compatible = "ns16550a"}},
        {.name = "spi0", .class_id = DEVICE_CLASS_SPI, .bus = DEVICE_BUS_SPI, .device_id = 0U,
         .probe = driver_probe_noop, .probe_device = driver_probe_device_noop,
         .remove_device = driver_remove_noop, .suspend = driver_suspend_noop, .resume = driver_resume_noop,
         .irq_handler = driver_irq_noop, .ctx = NULL,
         .match = {.bus = DEVICE_BUS_SPI, .class_id = DEVICE_CLASS_SPI, .compatible = "generic-spi"}},
        {.name = "i2c0", .class_id = DEVICE_CLASS_I2C, .bus = DEVICE_BUS_I2C, .device_id = 0U,
         .probe = driver_probe_noop, .probe_device = driver_probe_device_noop,
         .remove_device = driver_remove_noop, .suspend = driver_suspend_noop, .resume = driver_resume_noop,
         .irq_handler = driver_irq_noop, .ctx = NULL,
         .match = {.bus = DEVICE_BUS_I2C, .class_id = DEVICE_CLASS_I2C, .compatible = "generic-i2c"}},
        {.name = "sdmmc0", .class_id = DEVICE_CLASS_SDMMC, .bus = DEVICE_BUS_SDMMC, .device_id = 0U,
         .probe = driver_probe_noop, .probe_device = driver_probe_device_noop,
         .remove_device = driver_remove_noop, .suspend = driver_suspend_noop, .resume = driver_resume_noop,
         .irq_handler = driver_irq_noop, .ctx = NULL,
         .match = {.bus = DEVICE_BUS_SDMMC, .class_id = DEVICE_CLASS_SDMMC, .compatible = "generic-sdmmc"}},
        {.name = "eth-pci", .class_id = DEVICE_CLASS_ETHERNET, .bus = DEVICE_BUS_PCI, .device_id = 0U,
         .probe = driver_probe_noop, .probe_device = driver_probe_device_noop,
         .remove_device = driver_remove_noop, .suspend = driver_suspend_noop, .resume = driver_resume_noop,
         .irq_handler = driver_irq_noop, .ctx = NULL,
         .match = {.bus = DEVICE_BUS_PCI, .class_id = DEVICE_CLASS_ETHERNET, .class_code = 0x02}},
        {.name = "eth-usb-cdc-ecm", .class_id = DEVICE_CLASS_ETHERNET, .bus = DEVICE_BUS_USB, .device_id = 1U,
         .probe = driver_probe_noop, .probe_device = driver_probe_device_noop,
         .remove_device = driver_remove_noop, .suspend = driver_suspend_noop, .resume = driver_resume_noop,
         .irq_handler = driver_irq_noop, .ctx = NULL,
         .match = {.bus = DEVICE_BUS_USB, .class_id = DEVICE_CLASS_ETHERNET, .compatible = "usb-cdc-ecm"}},
        {.name = "can0", .class_id = DEVICE_CLASS_CAN, .bus = DEVICE_BUS_CAN, .device_id = 0U,
         .probe = driver_probe_noop, .probe_device = driver_probe_device_noop,
         .remove_device = driver_remove_noop, .suspend = driver_suspend_noop, .resume = driver_resume_noop,
         .irq_handler = driver_irq_noop, .ctx = NULL,
         .match = {.bus = DEVICE_BUS_CAN, .class_id = DEVICE_CLASS_CAN, .compatible = "generic-can"}},
        {.name = "virtio-net0", .class_id = DEVICE_CLASS_VIRTIO, .bus = DEVICE_BUS_VIRTIO, .device_id = 0U,
         .probe = driver_probe_noop, .probe_device = driver_probe_device_noop,
         .remove_device = driver_remove_noop, .suspend = driver_suspend_noop, .resume = driver_resume_noop,
         .irq_handler = driver_irq_noop, .ctx = NULL,
         .match = {.bus = DEVICE_BUS_VIRTIO, .class_id = DEVICE_CLASS_VIRTIO, .vendor_id = 0x1AF4}},
    };

    static const device_desc_t devices[] = {
        {.bus = DEVICE_BUS_UART, .class_id = DEVICE_CLASS_UART, .device_id = 0U, .compatible = "ns16550a", .irq = 4U, .hotpluggable = 0U, .power_state = DEVICE_POWER_D0, .in_use = 1U},
        {.bus = DEVICE_BUS_SPI, .class_id = DEVICE_CLASS_SPI, .device_id = 0U, .compatible = "generic-spi", .irq = 5U, .hotpluggable = 0U, .power_state = DEVICE_POWER_D0, .in_use = 1U},
        {.bus = DEVICE_BUS_I2C, .class_id = DEVICE_CLASS_I2C, .device_id = 0U, .compatible = "generic-i2c", .irq = 6U, .hotpluggable = 0U, .power_state = DEVICE_POWER_D0, .in_use = 1U},
        {.bus = DEVICE_BUS_SDMMC, .class_id = DEVICE_CLASS_SDMMC, .device_id = 0U, .compatible = "generic-sdmmc", .irq = 9U, .hotpluggable = 1U, .power_state = DEVICE_POWER_D0, .in_use = 1U},
        {.bus = DEVICE_BUS_PCI, .class_id = DEVICE_CLASS_ETHERNET, .device_id = 0U, .vendor_id = 0x8086U, .product_id = 0x100EU, .class_code = 0x02U, .compatible = "pci-net", .irq = 10U, .hotpluggable = 1U, .power_state = DEVICE_POWER_D0,
         .rx_queue_count = 4U, .tx_queue_count = 4U,
         .security_flags = DEVICE_SECURITY_CAPABILITY_GATED | DEVICE_SECURITY_IOMMU_DMA_GUARD,
         .perf_flags = DEVICE_PERF_IRQ_AFFINITY | DEVICE_PERF_ZERO_COPY_RXTX | DEVICE_PERF_NAPI_BUDGETED_RX,
         .hw_feature_flags = DEVICE_HW_FEAT_MSI_X | DEVICE_HW_FEAT_RX_CSUM | DEVICE_HW_FEAT_TX_CSUM | DEVICE_HW_FEAT_TSO | DEVICE_HW_FEAT_RSS,
         .in_use = 1U},
        {.bus = DEVICE_BUS_USB, .class_id = DEVICE_CLASS_ETHERNET, .device_id = 1U, .compatible = "usb-cdc-ecm", .irq = 11U, .hotpluggable = 1U, .power_state = DEVICE_POWER_D0,
         .rx_queue_count = 1U, .tx_queue_count = 1U,
         .security_flags = DEVICE_SECURITY_CAPABILITY_GATED | DEVICE_SECURITY_IOMMU_DMA_GUARD | DEVICE_SECURITY_SIGNED_FW_ONLY,
         .perf_flags = DEVICE_PERF_IRQ_AFFINITY,
         .hw_feature_flags = DEVICE_HW_FEAT_RX_CSUM | DEVICE_HW_FEAT_TX_CSUM,
         .in_use = 1U},
        {.bus = DEVICE_BUS_CAN, .class_id = DEVICE_CLASS_CAN, .device_id = 0U, .compatible = "generic-can", .irq = 12U, .hotpluggable = 0U, .power_state = DEVICE_POWER_D0,
         .rx_queue_count = 1U, .tx_queue_count = 1U,
         .security_flags = DEVICE_SECURITY_CAPABILITY_GATED | DEVICE_SECURITY_IOMMU_DMA_GUARD,
         .perf_flags = DEVICE_PERF_IRQ_AFFINITY,
         .hw_feature_flags = DEVICE_HW_FEAT_CAN_FD,
         .in_use = 1U},
        {.bus = DEVICE_BUS_VIRTIO, .class_id = DEVICE_CLASS_VIRTIO, .device_id = 0U, .vendor_id = 0x1AF4U, .product_id = 0x1041U, .compatible = "virtio-net", .irq = 13U, .hotpluggable = 1U, .power_state = DEVICE_POWER_D0,
         .rx_queue_count = 2U, .tx_queue_count = 2U,
         .security_flags = DEVICE_SECURITY_CAPABILITY_GATED | DEVICE_SECURITY_IOMMU_DMA_GUARD,
         .perf_flags = DEVICE_PERF_IRQ_AFFINITY | DEVICE_PERF_ZERO_COPY_RXTX,
         .hw_feature_flags = DEVICE_HW_FEAT_VIRTIO_MODERN | DEVICE_HW_FEAT_RX_CSUM | DEVICE_HW_FEAT_TX_CSUM,
         .in_use = 1U},
    };

    static const device_mmio_window_t windows[] = {
        { .class_id = DEVICE_CLASS_ETHERNET, .device_id = 0U, .window_id = 0U, .phys_base = 0x40000000U, .virt_base = 0x8000000000U, .size_bytes = 0x10000U, .irq = 10U, .in_use = 1U },
        { .class_id = DEVICE_CLASS_ETHERNET, .device_id = 0U, .window_id = 1U, .phys_base = 0x40010000U, .virt_base = 0x8000010000U, .size_bytes = 0x10000U, .irq = 10U, .in_use = 1U },
        { .class_id = DEVICE_CLASS_VIRTIO, .device_id = 0U, .window_id = 0U, .phys_base = 0x40020000U, .virt_base = 0x8000020000U, .size_bytes = 0x10000U, .irq = 13U, .in_use = 1U },
    };

    for (size_t i = 0; i < sizeof(drivers)/sizeof(drivers[0]); ++i) {
        if (device_register_driver(&drivers[i]) != 0) {
            return -1;
        }
    }

    for (size_t i = 0; i < sizeof(devices)/sizeof(devices[0]); ++i) {
        if (device_register_bus_device(&devices[i]) != 0) {
            return -2;
        }
    }

    for (size_t i = 0; i < sizeof(windows)/sizeof(windows[0]); ++i) {
        if (device_register_mmio_window(&windows[i]) != 0) {
            return -3;
        }
    }

    if (device_bind_drivers() != 0) {
        return -4;
    }

    return 0;
}
