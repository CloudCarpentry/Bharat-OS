#ifndef BHARAT_KERNEL_BOOT_EVENTS_H
#define BHARAT_KERNEL_BOOT_EVENTS_H

#include "bharat/uapi/boot/boot_events.h"

void boot_events_publish(bh_boot_stage_t stage, uint8_t percent, bharat_status_t status, const char *label);

#endif /* BHARAT_KERNEL_BOOT_EVENTS_H */
