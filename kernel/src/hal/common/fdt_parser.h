#ifndef BHARAT_HAL_FDT_PARSER_H
#define BHARAT_HAL_FDT_PARSER_H

#include <stdint.h>
#include <stdbool.h>
#include "hal/hal_discovery.h"

#define FDT_MAGIC 0xd00dfeed

struct fdt_header {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
};

// Common devices discovered via FDT (Legacy, transitioning to system_discovery_t)
typedef struct {
    uint64_t uart_base;
    uint64_t uart_size;

    uint64_t gic_dist_base;
    uint64_t gic_dist_size;
    uint64_t gic_redist_base;
    uint64_t gic_redist_size;

    uint64_t plic_base;
    uint64_t plic_size;

    uint64_t clint_base;
    uint64_t clint_size;

    uint32_t clock_freq;
} fdt_devices_t;

// Validate FDT header
bool fdt_is_valid(const void* fdt_ptr);

// Parse the FDT and populate bharat_boot_info_t (cpus, memory) and devices
int fdt_parse(const void* fdt_ptr, void* boot_info_ptr, fdt_devices_t* out_devices);

// New FDT parser entry point populating the generic system discovery structures
int fdt_parse_discovery(const void* fdt_ptr, system_discovery_t* discovery);

#endif
