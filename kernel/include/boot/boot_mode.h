#ifndef BHARAT_BOOT_MODE_H
#define BHARAT_BOOT_MODE_H

typedef enum {
    BHARAT_BOOT_MODE_NORMAL = 0,
    BHARAT_BOOT_MODE_DIAGNOSTIC,
    BHARAT_BOOT_MODE_RECOVERY,
    BHARAT_BOOT_MODE_MANUFACTURING,
    BHARAT_BOOT_MODE_BENCHMARK,
    BHARAT_BOOT_MODE_LEGACY_BRINGUP
} bharat_boot_mode_t;

const char *bharat_boot_mode_name(bharat_boot_mode_t mode);
bharat_boot_mode_t bharat_boot_mode_select(void);

#endif
