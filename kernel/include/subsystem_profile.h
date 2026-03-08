#ifndef BHARAT_SUBSYSTEM_PROFILE_H
#define BHARAT_SUBSYSTEM_PROFILE_H

#include <stdint.h>

/*
 * Profile-aware subsystem capability matrix.
 *
 * Compile-time defaults come from:
 *   - device profile (BHARAT_PROFILE_*)
 *   - personality profile (BHARAT_PERSONALITY_*)
 *   - architecture (BHARAT_ARCH_*)
 *
 * Runtime activation can be tuned by boot hardware profile passed to
 * bharat_subsystems_init().
 */

typedef enum {
    BHARAT_STORAGE_BLOCK_LAYER = (1U << 0),
    BHARAT_STORAGE_NVME = (1U << 1),
    BHARAT_STORAGE_AHCI_SATA = (1U << 2),
    BHARAT_STORAGE_EMMC_SD = (1U << 3),
    BHARAT_STORAGE_FLASH_MTD = (1U << 4),
    BHARAT_STORAGE_RAMDISK = (1U << 5)
} bharat_storage_feature_t;

typedef enum {
    BHARAT_NET_LIGHTWEIGHT_STACK = (1U << 0),
    BHARAT_NET_FULL_TCPIP_STACK = (1U << 1),
    BHARAT_NET_ZERO_COPY_PATH = (1U << 2),
    BHARAT_NET_VIRTIO = (1U << 3),
    BHARAT_NET_ETHERNET = (1U << 4),
    BHARAT_NET_WIFI = (1U << 5),
    BHARAT_NET_TSN_EXT = (1U << 6),
    BHARAT_NET_CAN_EXT = (1U << 7),
    BHARAT_NET_ETHERCAT_EXT = (1U << 8)
} bharat_network_feature_t;

typedef struct {
    uint32_t features;
} bharat_storage_profile_t;

typedef struct {
    uint32_t features;
} bharat_network_profile_t;

void bharat_subsystems_init(const char *boot_hw_profile);
int bharat_subsystems_ready(void);
const bharat_storage_profile_t *bharat_storage_active_profile(void);
const bharat_network_profile_t *bharat_network_active_profile(void);

int bharat_storage_has(uint32_t feature);
int bharat_network_has(uint32_t feature);

#endif /* BHARAT_SUBSYSTEM_PROFILE_H */
