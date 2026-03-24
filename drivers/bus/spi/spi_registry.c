#include "spi_registry.h"
#include <stddef.h>

#define MAX_SPI_CONTROLLERS 8

static spi_controller_t *g_spi_controllers[MAX_SPI_CONTROLLERS] = {NULL};

void spi_registry_init(void) {
    for (int i = 0; i < MAX_SPI_CONTROLLERS; i++) {
        g_spi_controllers[i] = NULL;
    }
}

int spi_controller_register(spi_controller_t *ctrl) {
    if (!ctrl || !ctrl->ops || !ctrl->ops->transfer_one_message) {
        return -1; // Invalid argument
    }

    if (ctrl->bus_num < 0 || ctrl->bus_num >= MAX_SPI_CONTROLLERS) {
        return -1; // Out of bounds
    }

    if (g_spi_controllers[ctrl->bus_num] != NULL) {
        return -1; // Already registered
    }

    g_spi_controllers[ctrl->bus_num] = ctrl;
    return 0; // Success
}

void spi_controller_unregister(spi_controller_t *ctrl) {
    if (!ctrl || ctrl->bus_num < 0 || ctrl->bus_num >= MAX_SPI_CONTROLLERS) {
        return;
    }
    g_spi_controllers[ctrl->bus_num] = NULL;
}

spi_controller_t *spi_get_controller(int bus_num) {
    if (bus_num < 0 || bus_num >= MAX_SPI_CONTROLLERS) {
        return NULL;
    }
    return g_spi_controllers[bus_num];
}

spi_device_t *spi_device_create(spi_controller_t *ctrl, uint8_t chip_select, const char *name) {
    if (!ctrl || !name) return NULL;
    if (chip_select >= ctrl->num_chipselect) return NULL;

    // In a real implementation this would allocate memory via kalloc.
    return NULL;
}

void spi_device_destroy(spi_device_t *spi) {
    if (!spi) return;
    if (spi->controller && spi->controller->ops && spi->controller->ops->cleanup) {
        spi->controller->ops->cleanup(spi);
    }
    // Free the device memory once allocation is wired
}