#ifndef BHARAT_BOOT_MODE_H
#define BHARAT_BOOT_MODE_H

#include <stdbool.h>
#include <boot/boot_contract.h>

#ifdef __cplusplus
extern "C" {
#endif

struct boot_info;

typedef boot_mode_t bharat_boot_mode_t;

#define BHARAT_BOOT_MODE_NORMAL        BOOT_MODE_NORMAL
#define BHARAT_BOOT_MODE_DIAGNOSTIC    BOOT_MODE_DEBUG
#define BHARAT_BOOT_MODE_SELFTEST      BOOT_MODE_SELFTEST
#define BHARAT_BOOT_MODE_RECOVERY      BOOT_MODE_RECOVERY
#define BHARAT_BOOT_MODE_SAFE          BOOT_MODE_SAFE
#define BHARAT_BOOT_MODE_MANUFACTURING BOOT_MODE_PROVISIONING
#define BHARAT_BOOT_MODE_BENCHMARK     BOOT_MODE_BENCHMARK

#define BHARAT_BOOT_MODE_AUTOMOTIVE    BOOT_MODE_AUTOMOTIVE
#define BHARAT_BOOT_MODE_LEGACY_BRINGUP BOOT_MODE_LEGACY_BRINGUP

const char *bharat_boot_mode_name(bharat_boot_mode_t mode);

// Resolves boot mode from the canonical boot info command line / fallback
int boot_mode_resolve(const struct boot_info *bi, bharat_boot_mode_t *out_mode);

bool boot_mode_allows_selftests(bharat_boot_mode_t mode, const struct boot_info *bi);
bool boot_mode_allows_debug_args(bharat_boot_mode_t mode, const struct boot_info *bi);
bool boot_mode_should_skip_optional_drivers(bharat_boot_mode_t mode);

// Legacy compat for now until kernel main is fully adapted
bharat_boot_mode_t bharat_boot_mode_select(void);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_BOOT_MODE_H
