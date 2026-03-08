#ifndef BHARAT_DRIVERS_CAN_H
#define BHARAT_DRIVERS_CAN_H

#include <stdint.h>
#include <stdbool.h>

// Initialize the virtual CAN loopback driver
void can_loopback_init(void);

// Send a CAN frame through the loopback driver
bool can_loopback_send(uint32_t id, const uint8_t* data, uint8_t dlc);

#endif // BHARAT_DRIVERS_CAN_H
