#ifndef BHARAT_HAL_BOOT_H
#define BHARAT_HAL_BOOT_H

#include <stdint.h>
#include <stdbool.h>
#include "hal_secure_boot.h"

typedef enum {
    BHARAT_FIRMWARE_UNKNOWN = 0,
    BHARAT_FIRMWARE_ACPI,
    BHARAT_FIRMWARE_FDT,
    BHARAT_FIRMWARE_SBI,
    BHARAT_FIRMWARE_MULTIBOOT
} bharat_firmware_type_t;

typedef struct {
    uint32_t cpu_id;
    uint32_t apic_id; // or mp_id / hart_id
    uint32_t numa_node;
    bool is_bsp;
} bharat_cpu_info_t;

#define BHARAT_MAX_CPUS 256
#define BHARAT_MAX_MEM_REGIONS 64

typedef struct {
    uint64_t base;
    uint64_t size;
    uint32_t numa_node;
    uint32_t type; // e.g. RAM, Reserved
} bharat_mem_region_t;

typedef struct {
    bharat_firmware_type_t fw_type;
    uint32_t cpu_count;
    bharat_cpu_info_t cpus[BHARAT_MAX_CPUS];
    uint32_t mem_region_count;
    bharat_mem_region_t mem_regions[BHARAT_MAX_MEM_REGIONS];
    void* acpi_rsdp;
    void* fdt_base;
    bharat_trust_evidence_t trust_evidence;
    bharat_profile_toggles_t profile_toggles;
} bharat_boot_info_t;

// SMP CPU Boot States
typedef enum {
    BH_CPU_BOOT_OFFLINE = 0,
    BH_CPU_BOOT_STARTING,
    BH_CPU_BOOT_ARCH_READY,
    BH_CPU_BOOT_MEMORY_READY,
    BH_CPU_BOOT_IRQ_READY,
    BH_CPU_BOOT_TIMER_READY,
    BH_CPU_BOOT_URPC_READY,
    BH_CPU_BOOT_SCHED_READY,
    BH_CPU_BOOT_ONLINE,
    BH_CPU_BOOT_FAILED
} bh_cpu_boot_state_t;

// ARM64 AP Boot Context
typedef struct {
    uint64_t logical_cpu_id;
    uint64_t stack_top_phys;
    uint64_t cpu_local_phys;

    uint64_t kernel_root_phys;
    uint64_t ttbr0_root_phys;
    uint64_t tcr_el1;
    uint64_t mair_el1;
    uint64_t sctlr_el1;
    uint64_t vbar_el1;

    uint64_t boot_generation;
} __attribute__((aligned(64))) bh_arm64_ap_boot_context_t;

// SMP Primary Core Handlers
int bh_smp_boot_primary_init(const bharat_boot_info_t *boot);
int bh_smp_start_secondary_cpus(uint32_t requested_cpus);
uint32_t bh_smp_get_online_core_count(void);

// The canonical C secondary entry point for all architectures
void bh_secondary_cpu_entry(uint64_t context_phys) __attribute__((noreturn));

// Architecture specific entry point for secondary cores
void secondary_entry_arch_early(void);
// Architecture specific late init for secondary cores
void secondary_entry_arch_late(void);

// Common secondary entry point called by architecture specific code (deprecated/wrapper)
void secondary_entry_common(void);

// Start a specific CPU
int hal_boot_start_cpu(uint32_t cpu_id, uint64_t entry_point);

// Get global boot info populated by early architecture code
bharat_boot_info_t* hal_boot_get_info(void);

#endif
