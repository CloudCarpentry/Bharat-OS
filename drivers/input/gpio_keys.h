#ifndef BHARAT_DRIVER_GPIO_KEYS_H
#define BHARAT_DRIVER_GPIO_KEYS_H

#include "../include/driver_core.h"

int gpio_keys_init(void);
int gpio_keys_device_register(device_desc_t* dev);

#endif // BHARAT_DRIVER_GPIO_KEYS_H