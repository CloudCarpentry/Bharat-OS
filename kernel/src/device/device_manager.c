#include "device.h"
#include "kernel_safety.h"
#include "mm.h"
#include "advanced/algo_matrix.h"

#include <stddef.h>

#define MAX_DEV_DRIVERS 32U
#define MAX_MMIO_WINDOWS 64U
#define MAX_BUS_DEVICES 64U
#define MAX_BINDINGS 64U

typedef struct {
    uint8_t in_use;
    uint8_t driver_index;
    uint8_t device_index;
} device_binding_t;

static device_driver_t g_drivers[MAX_DEV_DRIVERS];
static device_mmio_window_t g_windows[MAX_MMIO_WINDOWS];
static device_desc_t g_bus_devices[MAX_BUS_DEVICES];
static device_binding_t g_bindings[MAX_BINDINGS];

static int is_network_class(device_class_t class_id) {
    return (class_id == DEVICE_CLASS_ETHERNET ||
            class_id == DEVICE_CLASS_CAN ||
            class_id == DEVICE_CLASS_VIRTIO);
}

static int str_eq(const char* a, const char* b) {
    if (a == b) return 1;
    if (!a || !b) return 0;
    while (*a && *b) { if (*a != *b) return 0; ++a; ++b; }
    return *a == *b;
}

static int device_match_driver(const device_driver_t* driver, const device_desc_t* dev) {
    if (!driver || !dev) {
        return 0;
    }

    if (driver->bus != dev->bus || driver->class_id != dev->class_id) {
        return 0;
    }

    if (driver->match.vendor_id != 0U && driver->match.vendor_id != dev->vendor_id) {
        return 0;
    }

    if (driver->match.product_id != 0U && driver->match.product_id != dev->product_id) {
        return 0;
    }

    if (driver->match.class_code != 0U && driver->match.class_code != dev->class_code) {
        return 0;
    }

    if (driver->match.subclass_code != 0U && driver->match.subclass_code != dev->subclass_code) {
        return 0;
    }

    if (driver->match.compatible && dev->compatible) {
        if (!str_eq(driver->match.compatible, dev->compatible)) {
            return 0;
        }
    }

    return 1;
}

int device_framework_init(void) {
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_drivers); ++i) {
        g_drivers[i].name = NULL;
        g_drivers[i].probe = NULL;
        g_drivers[i].probe_device = NULL;
        g_drivers[i].remove_device = NULL;
        g_drivers[i].suspend = NULL;
        g_drivers[i].resume = NULL;
        g_drivers[i].irq_handler = NULL;
        g_drivers[i].ctx = NULL;
        g_drivers[i].device_id = 0U;
        g_drivers[i].class_id = DEVICE_CLASS_UART;
        g_drivers[i].bus = DEVICE_BUS_PLATFORM_MMIO;
        g_drivers[i].match.compatible = NULL;
    }

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_windows); ++i) {
        g_windows[i].in_use = 0U;
    }

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_bus_devices); ++i) {
        g_bus_devices[i].in_use = 0U;
        g_bus_devices[i].compatible = NULL;
    }

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_bindings); ++i) {
        g_bindings[i].in_use = 0U;
        g_bindings[i].driver_index = 0U;
        g_bindings[i].device_index = 0U;
    }

    return 0;
}

int device_register_driver(const device_driver_t* driver) {
    if (!driver || !driver->name) {
        return -1;
    }

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_drivers); ++i) {
        if (!g_drivers[i].name) {
            g_drivers[i] = *driver;
            return 0;
        }
    }

    return -2;
}

int device_register_bus_device(const device_desc_t* dev) {
    device_desc_t sanitized;

    if (!dev) {
        return -1;
    }

    sanitized = *dev;

    if (is_network_class(sanitized.class_id)) {
        if ((sanitized.security_flags & DEVICE_SECURITY_CAPABILITY_GATED) == 0U ||
            (sanitized.security_flags & DEVICE_SECURITY_IOMMU_DMA_GUARD) == 0U) {
            return -4;
        }

        if (sanitized.rx_queue_count == 0U) {
            sanitized.rx_queue_count = 1U;
        }
        if (sanitized.tx_queue_count == 0U) {
            sanitized.tx_queue_count = 1U;
        }

        if (sanitized.rx_queue_count > 4U) {
            sanitized.rx_queue_count = 4U;
        }
        if (sanitized.tx_queue_count > 4U) {
            sanitized.tx_queue_count = 4U;
        }
    }

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_bus_devices); ++i) {
        if (g_bus_devices[i].in_use != 0U &&
            g_bus_devices[i].bus == sanitized.bus &&
            g_bus_devices[i].device_id == sanitized.device_id) {
            return -3;
        }
    }

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_bus_devices); ++i) {
        if (g_bus_devices[i].in_use == 0U) {
            g_bus_devices[i] = sanitized;
            g_bus_devices[i].in_use = 1U;
            return 0;
        }
    }

    return -2;
}

int device_bind_drivers(void) {
    for (size_t d = 0; d < BHARAT_ARRAY_SIZE(g_drivers); ++d) {
        if (!g_drivers[d].name) {
            continue;
        }

        for (size_t dev = 0; dev < BHARAT_ARRAY_SIZE(g_bus_devices); ++dev) {
            if (g_bus_devices[dev].in_use == 0U) {
                continue;
            }

            if (!device_match_driver(&g_drivers[d], &g_bus_devices[dev])) {
                continue;
            }

            int already_bound = 0;
            for (size_t b = 0; b < BHARAT_ARRAY_SIZE(g_bindings); ++b) {
                if (g_bindings[b].in_use != 0U &&
                    g_bindings[b].driver_index == d &&
                    g_bindings[b].device_index == dev) {
                    already_bound = 1;
                    break;
                }
            }
            if (already_bound) {
                continue;
            }

            void* bound_ctx = g_drivers[d].ctx;
            if (g_drivers[d].probe_device) {
                if (g_drivers[d].probe_device(&g_bus_devices[dev], &bound_ctx) != 0) {
                    continue;
                }
            } else if (g_drivers[d].probe) {
                if (g_drivers[d].probe() != 0) {
                    continue;
                }
            }

            for (size_t b = 0; b < BHARAT_ARRAY_SIZE(g_bindings); ++b) {
                if (g_bindings[b].in_use == 0U) {
                    g_bindings[b].in_use = 1U;
                    g_bindings[b].driver_index = (uint8_t)d;
                    g_bindings[b].device_index = (uint8_t)dev;
                    g_drivers[d].ctx = bound_ctx;
                    break;
                }
            }
        }
    }

    return 0;
}

int device_hotplug_add(const device_desc_t* dev) {
    int rc = device_register_bus_device(dev);
    if (rc != 0) {
        return rc;
    }

    return device_bind_drivers();
}

int device_hotplug_remove(device_bus_t bus, uint32_t device_id) {
    for (size_t dev = 0; dev < BHARAT_ARRAY_SIZE(g_bus_devices); ++dev) {
        if (g_bus_devices[dev].in_use != 0U &&
            g_bus_devices[dev].bus == bus &&
            g_bus_devices[dev].device_id == device_id) {

            for (size_t b = 0; b < BHARAT_ARRAY_SIZE(g_bindings); ++b) {
                if (g_bindings[b].in_use != 0U && g_bindings[b].device_index == dev) {
                    device_driver_t* drv = &g_drivers[g_bindings[b].driver_index];
                    if (drv->remove_device) {
                        (void)drv->remove_device(drv->ctx);
                    }
                    g_bindings[b].in_use = 0U;
                }
            }

            g_bus_devices[dev].in_use = 0U;
            return 0;
        }
    }

    return -1;
}

int device_set_power_state(device_bus_t bus, uint32_t device_id, device_power_state_t target_state) {
    for (size_t dev = 0; dev < BHARAT_ARRAY_SIZE(g_bus_devices); ++dev) {
        if (g_bus_devices[dev].in_use == 0U ||
            g_bus_devices[dev].bus != bus ||
            g_bus_devices[dev].device_id != device_id) {
            continue;
        }

        for (size_t b = 0; b < BHARAT_ARRAY_SIZE(g_bindings); ++b) {
            if (g_bindings[b].in_use == 0U || g_bindings[b].device_index != dev) {
                continue;
            }

            device_driver_t* drv = &g_drivers[g_bindings[b].driver_index];
            if (target_state == DEVICE_POWER_D0) {
                if (drv->resume) {
                    (void)drv->resume(drv->ctx);
                }
            } else if (drv->suspend) {
                (void)drv->suspend(drv->ctx, target_state);
            }
        }

        g_bus_devices[dev].power_state = target_state;
        return 0;
    }

    return -1;
}

int device_register_mmio_window(const device_mmio_window_t* window) {
    if (!window || window->size_bytes == 0U) {
        return -1;
    }

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_windows); ++i) {
        if (g_windows[i].in_use == 0U) {
            g_windows[i] = *window;
            g_windows[i].in_use = 1U;

            #ifndef TESTING
            uint32_t num_pages = (window->size_bytes + PAGE_SIZE - 1) / PAGE_SIZE;
            for (uint32_t p = 0; p < num_pages; p++) {
                vmm_map_device_mmio(
                    window->virt_base + (p * PAGE_SIZE),
                    window->phys_base + (p * PAGE_SIZE),
                    NULL,
                    0
                );
            }
            #endif

            return 0;
        }
    }

    return -2;
}

int device_lookup_mmio_window_l0(uint32_t class_id,
                                 uint32_t device_id,
                                 uint32_t window_id,
                                 void* out_window) {
    device_mmio_window_t* out = (device_mmio_window_t*)out_window;
    if (!out) {
        return -1;
    }

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_windows); ++i) {
        if (g_windows[i].in_use != 0U &&
            g_windows[i].class_id == class_id &&
            g_windows[i].device_id == device_id &&
            g_windows[i].window_id == window_id) {
            *out = g_windows[i];
            return 0;
        }
    }

    return -2;
}

int device_lookup_mmio_window_l1(uint32_t class_id,
                                 uint32_t device_id,
                                 uint32_t window_id,
                                 void* out_window) {
    device_mmio_window_t* out = (device_mmio_window_t*)out_window;
    if (!out) {
        return -1;
    }

    for (int i = BHARAT_ARRAY_SIZE(g_windows) - 1; i >= 0; --i) {
        if (g_windows[i].in_use != 0U &&
            g_windows[i].class_id == class_id &&
            g_windows[i].device_id == device_id &&
            g_windows[i].window_id == window_id) {
            *out = g_windows[i];
            return 0;
        }
    }

    return -2;
}

int device_lookup_mmio_window(device_class_t class_id,
                              uint32_t device_id,
                              uint32_t window_id,
                              device_mmio_window_t* out_window) {
    if (g_search_ops.device_lookup_mmio_window) {
        return g_search_ops.device_lookup_mmio_window(class_id, device_id, window_id, (void*)out_window);
    }

    return device_lookup_mmio_window_l0(class_id, device_id, window_id, (void*)out_window);
}

int device_dispatch_irq(uint32_t irq) {
    for (size_t b = 0; b < BHARAT_ARRAY_SIZE(g_bindings); ++b) {
        if (g_bindings[b].in_use == 0U) {
            continue;
        }

        device_driver_t* drv = &g_drivers[g_bindings[b].driver_index];
        device_desc_t* dev = &g_bus_devices[g_bindings[b].device_index];

        if (drv->irq_handler && dev->irq == irq) {
            drv->irq_handler(irq, drv->ctx);
        }
    }

    return 0;
}
