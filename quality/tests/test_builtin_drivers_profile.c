#include <assert.h>
#include <stdio.h>

#include "../kernel/include/device.h"
#include "../kernel/include/subsystem_profile.h"

uint32_t hal_cpu_get_id(void) { return 0U; }

int main(void) {
    assert(device_framework_init() == 0);

    bharat_subsystems_init("vm");
    assert(device_register_builtin_drivers() == 0);

    assert(device_driver_registered(DEVICE_CLASS_UART, 0U) == 1);
    assert(device_driver_registered(DEVICE_CLASS_VIRTIO_NET, 0U) == 1);
    assert(device_driver_registered(DEVICE_CLASS_WIFI, 0U) == 0);
    assert(device_driver_registered(DEVICE_CLASS_AHCI, 0U) == 0);

    printf("test_builtin_drivers_profile passed\n");
    return 0;
}
