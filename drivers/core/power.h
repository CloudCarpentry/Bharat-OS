#ifndef BHARAT_DRIVER_POWER_H
#define BHARAT_DRIVER_POWER_H

#include "driver_core.h"

int driver_power_suspend_device(device_desc_t* dev);
int driver_power_resume_device(device_desc_t* dev);

#endif // BHARAT_DRIVER_POWER_H