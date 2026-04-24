#ifndef BHARAT_DRIVER_GPIO_CORE_H
#define BHARAT_DRIVER_GPIO_CORE_H

#include "../../include/driver_core.h"

// Define a simple struct for a GPIO controller driver data
typedef struct {
    int base_pin;
    int num_pins;
    // Real implementation would have read/write callbacks here
    int (*set_direction)(device_desc_t* dev, int pin_offset, int dir);
    int (*set_value)(device_desc_t* dev, int pin_offset, int value);
    int (*get_value)(device_desc_t* dev, int pin_offset);
} gpio_controller_data_t;

int gpio_core_init(void);

// Register a GPIO controller
int gpio_controller_register(device_desc_t* dev);

#endif // BHARAT_DRIVER_GPIO_CORE_H