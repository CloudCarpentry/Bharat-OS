#ifndef BHARAT_BOOT_BH_BOOT_CONTEXT_H
#define BHARAT_BOOT_BH_BOOT_CONTEXT_H

#include <stdint.h>
#include "boot_contract.h"
#include "boot_info.h" // For some common structures if needed, though we will define our own here per requirements

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BH_ARCH_ID_UNKNOWN = 0,
    BH_ARCH_ID_X86_64 = 1,
    BH_ARCH_ID_ARM64 = 2,
    BH_ARCH_ID_ARM32 = 3,
    BH_ARCH_ID_RISCV64 = 4,
    BH_ARCH_ID_RISCV32 = 5
} bh_arch_id_t;

typedef enum {
    BH_PLATFORM_ID_UNKNOWN = 0,
    BH_PLATFORM_ID_QEMU_VIRT = 1,
    BH_PLATFORM_ID_GCP = 2,
    BH_PLATFORM_ID_AWS = 3,
    BH_PLATFORM_ID_AZURE = 4,
    BH_PLATFORM_ID_RPI4 = 5
} bh_platform_id_t;

typedef enum {
    BH_PROFILE_ID_UNKNOWN = 0,
    BH_PROFILE_ID_CLOUD = 1,
    BH_PROFILE_ID_DRONE = 2,
    BH_PROFILE_ID_EDGE = 3
} bh_profile_id_t;

struct bh_memory_map {
    boot_memory_region_t regions[BHARAT_BOOT_MAX_MEM_REGIONS];
    uint32_t region_count;
};

struct bh_cpu_boot_info {
    uint64_t boot_hart_id;
    uint32_t cpu_count;
};

struct bh_fw_info {
    bool has_acpi;
    bool has_dtb;
    bool has_smbios;
};

struct bh_hw_desc {
    bool has_pci;
    bool has_msix;
};

struct bh_module_desc {
    uint64_t phys_start;
    uint64_t size;
    const char *name;
};

struct bh_boot_manifest {
    uint32_t version;
    char target_name[64];
};

struct bh_framebuffer_info {
    uint64_t phys_base;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
};

struct bh_measurement_log {
    uint32_t entry_count;
    // ...
};

struct bh_boot_context {
    uint32_t version;
    uint32_t size;

    bh_arch_id_t       arch;
    bh_platform_id_t   platform;
    bh_profile_id_t    profile;

    uint64_t boot_id;
    uint32_t boot_reason;
    uint32_t boot_flags;

    struct bh_memory_map memory;
    struct bh_cpu_boot_info cpu;

    struct bh_fw_info firmware;
    struct bh_hw_desc hardware;

    struct bh_module_desc modules[BHARAT_BOOT_MAX_MODULES];
    uint32_t module_count;

    struct bh_boot_manifest *manifest;

    void *acpi_rsdp;
    void *dtb;

    struct bh_framebuffer_info framebuffer;

    struct bh_measurement_log *measurements;
};

#ifdef __cplusplus
}
#endif

#endif // BHARAT_BOOT_BH_BOOT_CONTEXT_H
