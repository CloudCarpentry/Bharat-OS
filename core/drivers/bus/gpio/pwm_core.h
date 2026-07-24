#ifndef BHARAT_DRIVER_PWM_CORE_H
#define BHARAT_DRIVER_PWM_CORE_H

#include "../../include/driver_core.h"

int pwm_core_init(void);
int pwm_controller_register(device_desc_t* dev);

#endif // BHARAT_DRIVER_PWM_CORE_H