#ifndef BHARAT_DRIVER_DEVICE_REGISTRY_H
#define BHARAT_DRIVER_DEVICE_REGISTRY_H

#include "driver_core.h"

int device_registry_init(void);
int device_register(device_desc_t* dev);
void device_unregister(device_desc_t* dev);
device_desc_t* device_find_by_name(const char* name);

#endif // BHARAT_DRIVER_DEVICE_REGISTRY_H