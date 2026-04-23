#ifndef BHARAT_DRIVER_PINCTRL_CORE_H
#define BHARAT_DRIVER_PINCTRL_CORE_H

#include "../../include/driver_core.h"

int pinctrl_core_init(void);
int pinctrl_controller_register(device_desc_t* dev);

#endif // BHARAT_DRIVER_PINCTRL_CORE_H