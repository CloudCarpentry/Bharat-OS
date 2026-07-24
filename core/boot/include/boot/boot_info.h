#ifndef BHARAT_BOOT_BOOT_INFO_H
#define BHARAT_BOOT_BOOT_INFO_H

#include "boot_contract.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// The canonical boot info contract
typedef struct boot_info {
    uint32_t magic;
    uint32_t version;

    boot_source_t source;
    boot_arch_t arch;

    // Command line
    char cmdline[BHARAT_BOOT_CMDLINE_MAX_LEN];

    // Memory
    boot_memory_region_t mem_regions[BHARAT_BOOT_MAX_MEM_REGIONS];
    uint32_t mem_region_count;

    // Kernel boundaries
    uint64_t kernel_phys_start;
    uint64_t kernel_phys_end;

    // Modules
    boot_module_t modules[BHARAT_BOOT_MAX_MODULES];
    uint32_t module_count;

    // Mode and Status
    boot_mode_t selected_mode;
    boot_security_state_t security_state;
    boot_security_info_t security_info;

    // Flags for degraded mode tracking
    bool is_degraded;
    uint32_t degraded_reasons_mask; // Bitmask of boot_errno values or similar indicating non-fatal failures

    // Console and Firmware Handoff
    boot_console_info_t console;
    boot_firmware_info_t firmware;

    // CPU context / Topology
    uint64_t boot_cpu_id; // Hart ID / APIC ID / MPIDR

    // Validation Status
    bool is_validated;

} boot_info_t;

// API to populate boot_info
void boot_info_init(boot_info_t *bi);

int boot_info_add_mem_region(boot_info_t *bi, uint64_t phys_start, uint64_t size, boot_mem_type_t type);

int boot_info_add_module(boot_info_t *bi, uint64_t phys_start, uint64_t size, const char *name);

int boot_info_set_cmdline(boot_info_t *bi, const char *cmdline, size_t len);

int boot_info_finalize(boot_info_t *bi);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_BOOT_BOOT_INFO_H
