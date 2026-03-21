#ifndef BHARAT_DRIVERS_CAN_H
#define BHARAT_DRIVERS_CAN_H

#include <stdint.h>
#include <stdbool.h>

// Initialize the virtual CAN loopback driver
void can_loopback_init(void);

// Send a CAN frame through the loopback driver
bool can_loopback_send(uint32_t id, const uint8_t* data, uint8_t dlc);
// Receive a CAN frame from the loopback queue
bool can_loopback_receive(uint32_t* id, uint8_t* data, uint8_t* dlc);
// Return number of pending frames in loopback queue
uint8_t can_loopback_pending(void);
// Return number of dropped frames due to queue saturation
uint32_t can_loopback_drop_count(void);
// Reset queue and statistics
void can_loopback_reset(void);

#endif // BHARAT_DRIVERS_CAN_H
