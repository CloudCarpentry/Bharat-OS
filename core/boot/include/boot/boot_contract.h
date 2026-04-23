#ifndef BHARAT_BOOT_BOOT_CONTRACT_H
#define BHARAT_BOOT_BOOT_CONTRACT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Limits
#define BHARAT_BOOT_MAX_MEM_REGIONS 128
#define BHARAT_BOOT_MAX_MODULES 16
#define BHARAT_BOOT_CMDLINE_MAX_LEN 1024

// Canonical Magic for boot_info validation
#define BHARAT_BOOT_INFO_MAGIC 0xB4A2A705

// Stable enumerations for boot sources
typedef enum {
    BOOT_SOURCE_UNKNOWN = 0,
    BOOT_SOURCE_MULTIBOOT2,
    BOOT_SOURCE_UEFI,
    BOOT_SOURCE_UBOOT_FDT,
    BOOT_SOURCE_OPENSBI_FDT,
    BOOT_SOURCE_LEGACY_LOADER
} boot_source_t;

// Stable enumerations for architectures
typedef enum {
    BOOT_ARCH_UNKNOWN = 0,
    BOOT_ARCH_X86_64,
    BOOT_ARCH_ARM64,
    BOOT_ARCH_ARM32,
    BOOT_ARCH_RISCV64,
    BOOT_ARCH_RISCV32
} boot_arch_t;

// Mode definitions
typedef enum {
    BOOT_MODE_NORMAL = 0,
    BOOT_MODE_DEBUG,
    BOOT_MODE_SELFTEST,
    BOOT_MODE_RECOVERY,
    BOOT_MODE_SAFE,
    BOOT_MODE_PROVISIONING,
    BOOT_MODE_BENCHMARK,
    BOOT_MODE_LEGACY_BRINGUP
} boot_mode_t;

// Security posture definitions
typedef enum {
    BOOT_SECURITY_UNKNOWN = 0,
    BOOT_SECURITY_INSECURE,
    BOOT_SECURITY_SECURE_UNVERIFIED,
    BOOT_SECURITY_SECURE_VERIFIED,
    BOOT_SECURITY_MEASURED,
    BOOT_SECURITY_MEASURED_AND_VERIFIED
} boot_security_state_t;

// Memory region types
typedef enum {
    BOOT_MEM_UNKNOWN = 0,
    BOOT_MEM_USABLE,
    BOOT_MEM_RESERVED,
    BOOT_MEM_ACPI_RECLAIM,
    BOOT_MEM_ACPI_NVS,
    BOOT_MEM_BAD,
    BOOT_MEM_KERNEL,
    BOOT_MEM_INITRD,
    BOOT_MEM_FRAMEBUFFER,
    BOOT_MEM_FIRMWARE
} boot_mem_type_t;

typedef struct {
    uint64_t phys_start;
    uint64_t size;
    boot_mem_type_t type;
} boot_memory_region_t;

typedef struct {
    uint64_t phys_start;
    uint64_t size;
    const char *name; // Points to a string inside the module or bootloader allocation
} boot_module_t;

// Console types
typedef enum {
    BOOT_CONSOLE_NONE = 0,
    BOOT_CONSOLE_SERIAL,
    BOOT_CONSOLE_FRAMEBUFFER
} boot_console_type_t;

typedef struct {
    boot_console_type_t type;

    // Valid if type == BOOT_CONSOLE_SERIAL
    uint64_t serial_phys_base;
    uint32_t serial_baud_rate;

    // Valid if type == BOOT_CONSOLE_FRAMEBUFFER
    uint64_t fb_phys_base;
    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_pitch;
    uint32_t fb_bpp;
} boot_console_info_t;

typedef struct {
    void *fdt_ptr;
    void *acpi_rsdp_ptr;
    bool has_rng_seed;
    uint64_t rng_seed_phys;
    uint32_t rng_seed_size;
} boot_firmware_info_t;

typedef struct {
    bool secure_boot_present;
    bool secure_boot_verified;
    bool measured_boot_present;
    bool tpm_present;
    bool event_log_present;
    bool kernel_measurement_present;
    bool initrd_measurement_present;
    bool firmware_measurement_present;
} boot_security_info_t;

#ifdef __cplusplus
}
#endif

#endif // BHARAT_BOOT_BOOT_CONTRACT_H
