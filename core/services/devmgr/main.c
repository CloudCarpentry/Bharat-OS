#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <bharat/syscalls.h>
#include "../../drivers/core/device_registry.h"
#include "../../drivers/core/driver_registry.h"
#include "../../drivers/core/binding.h"
#include "../../drivers/core/event.h"

/**
 * @file main.c
 * @brief devmgr - Central device enumeration and lifecycle service.
 */

extern int virt_accel_register_device(void);

static void devmgr_enumerate_devices(void) {
    // Rely on canonical device registry instead of a local mock array
    // Device drivers (like the virt_accel provider) will register themselves
    // and the core framework tracks them.
    device_registry_init();
    driver_registry_init();

    // Call the device registration of virtual accel
    virt_accel_register_device();
}

static uint32_t devmgr_query_capabilities(device_desc_t *dev) {
    if (!dev) return 0;
    return dev->capability_flags;
}

static bool devmgr_check_dma_capability(device_desc_t *dev) {
    if (!dev) return false;
    // Example abstraction: assume if DMA bit 0 is set it is dma_capable
    return (dev->capability_flags & 0x1) != 0;
}

static void devmgr_associate_iommu(device_desc_t *dev, uint32_t domain) {
    if (!dev) return;
    // Store domain association logically. In a real scenario, call HAL/IOMMU setup
    (void)domain;
}

static bool devmgr_bind_driver(device_desc_t *dev) {
    if (!dev) return false;
    return device_binding_get_for_device(dev) != NULL;
}

static void devmgr_handle_device_fault(device_desc_t *dev) {
    if (!dev) return;
    device_binding_t *binding = device_binding_get_for_device(dev);
    if (binding && binding->driver && binding->driver->fault) {
        binding->driver->fault(dev);
        binding->state = DRIVER_STATE_FAILED;
    }
}

static void devmgr_device_lifecycle_update(device_desc_t *dev, driver_lifecycle_state_t new_state) {
    if (!dev) return;
    device_binding_t *binding = device_binding_get_for_device(dev);
    if (binding) {
        binding->state = new_state;
    }
}

void init_devmgr(void) {
    devmgr_enumerate_devices();

    device_desc_t* dev = device_find_by_name("virt_accel_0");

    if (dev) {
        uint32_t caps = devmgr_query_capabilities(dev);
        (void)caps;

        devmgr_device_lifecycle_update(dev, DRIVER_STATE_MATCHED);

        bool can_dma = devmgr_check_dma_capability(dev);
        (void)can_dma;

        devmgr_associate_iommu(dev, 1);

        devmgr_bind_driver(dev);

        // don't fault it manually in production setup, wait for event
        // devmgr_handle_device_fault(dev);
    }
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    init_devmgr();

    // Main event loop
    while (true) {
        // Wait for hotplug events, device reset requests, or driver bind requests
        bharat_sched_yield();
    }

    return 0;
}
