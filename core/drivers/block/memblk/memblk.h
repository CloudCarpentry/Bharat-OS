#ifndef BHARAT_DRIVER_BLOCK_MEMBLK_H
#define BHARAT_DRIVER_BLOCK_MEMBLK_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void memblk_init(void);
void memblk_deinit(void);

// For testing purposes
void memblk_inject_error(bool enable);
void memblk_inject_delay(uint32_t delay_loops);
void memblk_tick(void);

// Reboot-persistence & fault-injection helpers
void memblk_attach_backing(uint32_t device_id, bool enable);
void memblk_power_cycle(uint32_t device_id);
void memblk_reopen(uint32_t device_id);

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_DRIVER_BLOCK_MEMBLK_H */
