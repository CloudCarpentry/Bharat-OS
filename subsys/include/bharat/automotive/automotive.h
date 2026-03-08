#ifndef BHARAT_SUBSYS_AUTOMOTIVE_H
#define BHARAT_SUBSYS_AUTOMOTIVE_H

#include <stdint.h>
#include <stdbool.h>

// Initialize the automotive subsystem (e.g., CAN routing, ECU discovery)
void subsys_automotive_init(void);

// Example subsystem interface
bool subsys_automotive_send_can_frame(uint32_t id, const uint8_t* data, uint8_t dlc);

#endif // BHARAT_SUBSYS_AUTOMOTIVE_H
