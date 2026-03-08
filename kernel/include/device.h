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
} device_class_t;

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
    const char* name;
    device_class_t class_id;
    uint32_t device_id;
    int (*probe)(void);
    void (*irq_handler)(uint32_t irq, void* ctx);
    void* ctx;
} device_driver_t;

int device_framework_init(void);
int device_register_driver(const device_driver_t* driver);
int device_register_mmio_window(const device_mmio_window_t* window);
int device_lookup_mmio_window(device_class_t class_id,
                              uint32_t device_id,
                              uint32_t window_id,
                              device_mmio_window_t* out_window);
int device_dispatch_irq(uint32_t irq);

int device_register_builtin_drivers(void);

#endif // BHARAT_DEVICE_H

int pci_discover_nic(device_mmio_window_t* rx_window, device_mmio_window_t* tx_window);
