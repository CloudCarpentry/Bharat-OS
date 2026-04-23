#ifndef BHARAT_DRIVER_DRIVER_REGISTRY_H
#define BHARAT_DRIVER_DRIVER_REGISTRY_H

#include "driver_core.h"

int driver_registry_init(void);
int driver_register(driver_desc_t* drv);
void driver_unregister(driver_desc_t* drv);
driver_desc_t** driver_registry_get_all(int* count_out);

#endif // BHARAT_DRIVER_DRIVER_REGISTRY_H