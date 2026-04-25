#ifndef BHARAT_UAPI_SYSTEM_OTA_METADATA_H
#define BHARAT_UAPI_SYSTEM_OTA_METADATA_H

#include <stdint.h>

/**
 * @file ota_metadata.h
 * @brief Contract for Tiny/IoT update profiles.
 */

typedef enum {
    BH_OTA_SLOT_EMPTY = 0,
    BH_OTA_SLOT_VALID,
    BH_OTA_SLOT_INVALID,
    BH_OTA_SLOT_UPDATING,
    BH_OTA_SLOT_PENDING_REBOOT
} bh_ota_slot_state_t;

typedef struct {
    uint32_t magic;           // BH_OTA_MAGIC
    uint32_t version;         // Contract version
    uint32_t active_slot;     // 0 for A, 1 for B

    struct {
        bh_ota_slot_state_t state;
        uint32_t version;
        uint8_t checksum[32]; // SHA-256
        uint64_t install_time;
    } slots[2];

    uint32_t crc32;
} bh_ota_metadata_t;

#define BH_OTA_MAGIC 0x42484f54 // "BHOT"

#endif // BHARAT_UAPI_SYSTEM_OTA_METADATA_H
