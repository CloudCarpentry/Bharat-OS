#include "device.h"
#include "subsystem_profile.h"

#include <stddef.h>

static int driver_probe_noop(void) {
    return 0;
}

static void driver_irq_noop(uint32_t irq, void* ctx) {
    (void)irq;
    (void)ctx;
}

static int register_driver(const char* name, device_class_t class_id, uint32_t device_id) {
    device_driver_t driver = {
        .name = name,
        .class_id = class_id,
        .device_id = device_id,
        .probe = driver_probe_noop,
        .irq_handler = driver_irq_noop,
        .ctx = NULL,
    };
    return device_register_driver(&driver);
}

static int register_mmio(device_class_t class_id,
                         uint32_t device_id,
                         uint32_t window_id,
                         phys_addr_t phys,
                         virt_addr_t virt,
                         uint32_t size,
                         uint32_t irq) {
    device_mmio_window_t window = {
        .class_id = class_id,
        .device_id = device_id,
        .window_id = window_id,
        .phys_base = phys,
        .virt_base = virt,
        .size_bytes = size,
        .irq = irq,
        .in_use = 1U,
    };
    return device_register_mmio_window(&window);
}

int device_register_builtin_drivers(void) {
    if (!bharat_subsystems_ready()) {
        bharat_subsystems_init("generic");
    }

    if (register_driver("uart0", DEVICE_CLASS_UART, 0U) != 0) {
        return -1;
    }
    if (register_driver("spi0", DEVICE_CLASS_SPI, 0U) != 0) {
        return -1;
    }
    if (register_driver("i2c0", DEVICE_CLASS_I2C, 0U) != 0) {
        return -1;
    }

    if (bharat_storage_has(BHARAT_STORAGE_EMMC_SD)) {
        if (register_driver("sdmmc0", DEVICE_CLASS_SDMMC, 0U) != 0) {
            return -2;
        }
    }

    if (bharat_storage_has(BHARAT_STORAGE_NVME)) {
        if (register_driver("nvme0", DEVICE_CLASS_NVME, 0U) != 0) {
            return -3;
        }
    }

    if (bharat_storage_has(BHARAT_STORAGE_AHCI_SATA)) {
        if (register_driver("ahci0", DEVICE_CLASS_AHCI, 0U) != 0) {
            return -4;
        }
    }

    if (bharat_storage_has(BHARAT_STORAGE_FLASH_MTD)) {
        if (register_driver("flash0", DEVICE_CLASS_FLASH, 0U) != 0) {
            return -5;
        }
    }

    if (bharat_storage_has(BHARAT_STORAGE_RAMDISK)) {
        if (register_driver("ramdisk0", DEVICE_CLASS_RAMDISK, 0U) != 0) {
            return -6;
        }
    }

    if (bharat_network_has(BHARAT_NET_ETHERNET)) {
        if (register_driver("eth0", DEVICE_CLASS_ETHERNET, 0U) != 0) {
            return -7;
        }
        if (register_mmio(DEVICE_CLASS_ETHERNET, 0U, 0U, 0x40000000U, 0x8000000000U, 0x10000U, 10U) != 0) {
            return -8;
        }
        if (register_mmio(DEVICE_CLASS_ETHERNET, 0U, 1U, 0x40010000U, 0x8000010000U, 0x10000U, 10U) != 0) {
            return -8;
        }
    }

    if (bharat_network_has(BHARAT_NET_VIRTIO)) {
        if (register_driver("virtio-net0", DEVICE_CLASS_VIRTIO_NET, 0U) != 0) {
            return -9;
        }
    }

    if (bharat_network_has(BHARAT_NET_WIFI)) {
        if (register_driver("wlan0", DEVICE_CLASS_WIFI, 0U) != 0) {
            return -10;
        }
    }

    return 0;
}
