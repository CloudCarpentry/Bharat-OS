#ifndef BHARAT_BOOT_INFO_H
#define BHARAT_BOOT_INFO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "bharat/display/boot_video.h"

// Define maximum number of memory regions for the common boot info
#define BHARAT_BOOT_MAX_MEM_REGIONS 128
#define BHARAT_BOOT_MAX_RESERVED_REGIONS 32
#define BHARAT_BOOT_MAX_MODULES 16
#define BHARAT_BOOT_CMDLINE_MAX_LEN 1024

typedef enum {
    BOOT_MEM_USABLE = 1,
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
} boot_mem_region_t;

typedef struct {
    uint64_t phys_start;
    uint64_t size;
    const char *name;
} boot_module_t;

typedef enum {
    BOOT_UI_NONE = 0,          // serial only
    BOOT_UI_TEXT,              // text console on display or serial
    BOOT_UI_SIMPLE_FB,         // framebuffer splash/progress
    BOOT_UI_EMBEDDED_UI,       // lightweight interactive early UI
    BOOT_UI_DEFERRED_GRAPHICS  // no early graphics; later compositor starts
} boot_ui_mode_t;

typedef enum {
    PROFILE_MINIMAL = 0,   // serial/text only
    PROFILE_EDGE,          // embedded UI allowed
    PROFILE_DESKTOP,       // compositor later
    PROFILE_DEV_VM,        // prefer simple graphics in VM
    PROFILE_KIOSK          // embedded full-screen UI
} system_profile_t;

// The normalized boot contract, passed from architecture-specific entry
// to the generic kernel core.
typedef struct boot_info {
    uint32_t magic; // e.g., 0xB4A2A705 or something to indicate validity

    // Command line
    char cmdline[BHARAT_BOOT_CMDLINE_MAX_LEN];

    // Memory map
    boot_mem_region_t mem_map[BHARAT_BOOT_MAX_MEM_REGIONS];
    uint32_t mem_map_count;

    // Boot modules (e.g., initrd, firmware blobs)
    boot_module_t modules[BHARAT_BOOT_MAX_MODULES];
    uint32_t module_count;

    // Kernel boundaries
    uint64_t kernel_phys_start;
    uint64_t kernel_phys_end;

    // Architecture-specific topology / identifiers
    uint64_t boot_cpu_id; // Hart ID, APIC ID, MPIDR, etc.

    // Display handoff metadata
    boot_video_handoff_t video;

    // Architecture/Firmware provenance hints
    bool booted_via_uefi;
    bool booted_via_fdt;
    bool booted_via_multiboot;

    // Extra feature flags or security measurements
    uint32_t feature_flags;

} boot_info_t;

// Boot Display Hooks inside Kernel
int boot_video_collect(boot_video_handoff_t *out);
int boot_video_validate(const boot_video_handoff_t *in);
int kernel_publish_boot_framebuffer(const boot_video_handoff_t *in, uint32_t *out_cap);
boot_ui_mode_t boot_ui_resolve_mode(system_profile_t profile);

#endif // BHARAT_BOOT_INFO_H
