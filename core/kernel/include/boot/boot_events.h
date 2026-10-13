/* SPDX-License-Identifier: MIT */
#ifndef BHARAT_KERNEL_BOOT_EVENTS_H
#define BHARAT_KERNEL_BOOT_EVENTS_H

#include "bharat/uapi/boot/boot_events.h"

#ifdef __cplusplus
extern "C" {
#endif

void boot_events_init(void);

void boot_events_record(bh_boot_stage_t stage,
                        bh_boot_status_t status,
                        const char *component,
                        const char *message,
                        int32_t error_code);

/* Legacy publisher hook */
void boot_events_publish(bh_boot_stage_t stage,
                         uint8_t percent,
                         bharat_status_t status,
                         const char *label);

void kernel_boot_events_get_snapshot(bh_boot_event_snapshot_t *snapshot);

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_KERNEL_BOOT_EVENTS_H */
