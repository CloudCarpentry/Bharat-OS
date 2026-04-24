#ifndef BHARAT_DRIVER_LED_CORE_H
#define BHARAT_DRIVER_LED_CORE_H

#include "../include/driver_core.h"

int led_core_init(void);
int led_device_register(device_desc_t* dev);

#endif // BHARAT_DRIVER_LED_CORE_H