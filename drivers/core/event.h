#ifndef BHARAT_DRIVER_EVENT_H
#define BHARAT_DRIVER_EVENT_H

#include "driver_core.h"

// Define a callback type for event listeners (e.g., devmgr)
typedef void (*driver_event_cb_t)(device_event_t* event);

int driver_event_register_listener(driver_event_cb_t cb);
void driver_event_emit(device_event_type_t type, device_desc_t* dev);

#endif // BHARAT_DRIVER_EVENT_H