#ifndef BHARAT_UAPI_BOOT_EVENTS_H
#define BHARAT_UAPI_BOOT_EVENTS_H

#include <stdint.h>
#include <bharat/uapi/service_status.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BH_BOOT_STAGE_EARLY,
    BH_BOOT_STAGE_HAL,
    BH_BOOT_STAGE_SECURITY,
    BH_BOOT_STAGE_MEMORY,
    BH_BOOT_STAGE_SCHEDULER,
    BH_BOOT_STAGE_SERVICES,
    BH_BOOT_STAGE_USERSPACE,
    BH_BOOT_STAGE_READY
} bh_boot_stage_t;

typedef struct {
    bh_boot_stage_t stage;
    uint8_t percent;
    bharat_status_t status;
    char label[128];
} bh_boot_event_t;

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_UAPI_BOOT_EVENTS_H */
