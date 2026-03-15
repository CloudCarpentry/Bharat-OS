#ifndef BHARAT_HAL_DISCOVERY_H
#define BHARAT_HAL_DISCOVERY_H

#include <stdint.h>
#include <stdbool.h>

#define BHARAT_MAX_NODES 16
#define BHARAT_MAX_CPUS 256
#define BHARAT_MAX_MEM_REGIONS 64
#define BHARAT_MAX_IRQ_CONTROLLERS 8
#define BHARAT_MAX_TIMERS 4
#define BHARAT_MAX_PCI_HOSTS 8
#define BHARAT_MAX_IOMMUS 4
#define BHARAT_MAX_PMUS 4

// --- System Topology ---

typedef struct {
    uint32_t cpu_id;
    uint32_t hw_id;     // APIC ID, MPIDR, or Hart ID
    uint32_t node_id;   // NUMA node
    bool is_bsp;
} cpu_topology_t;

typedef struct {
    uint64_t base;
    uint64_t size;
    uint32_t node_id;
    uint32_t type;      // 1 = RAM, 2 = Reserved, etc.
} mem_topology_t;

typedef struct {
    uint32_t node_id;
    uint32_t distance[BHARAT_MAX_NODES]; // Distance to other nodes
} numa_distance_t;

typedef struct {
    uint32_t cpu_count;
    cpu_topology_t cpus[BHARAT_MAX_CPUS];

    uint32_t mem_region_count;
    mem_topology_t mem_regions[BHARAT_MAX_MEM_REGIONS];

    uint32_t node_count;
    numa_distance_t nodes[BHARAT_MAX_NODES];
} system_topology_t;

// --- Interrupt Controllers ---

typedef enum {
    IRQ_CTRL_UNKNOWN = 0,
    IRQ_CTRL_APIC,      // x86_64 Local APIC
    IRQ_CTRL_IOAPIC,    // x86_64 IOAPIC
    IRQ_CTRL_GICV2,     // ARM GICv2
    IRQ_CTRL_GICV3,     // ARM GICv3 (Distributor/Redistributor)
    IRQ_CTRL_GIC_ITS,   // ARM GICv3 ITS
    IRQ_CTRL_PLIC,      // RISC-V PLIC
    IRQ_CTRL_AIA_APLIC, // RISC-V AIA APLIC
    IRQ_CTRL_AIA_IMSIC, // RISC-V AIA IMSIC
} irq_ctrl_type_t;

typedef struct {
    irq_ctrl_type_t type;
    uint64_t base;      // Base address (e.g., GICD or IOAPIC base)
    uint64_t size;
    uint64_t aux_base;  // e.g., GICR (Redistributor) base
    uint64_t aux_size;
    uint32_t id;        // Controller ID (e.g., IOAPIC ID)
    uint32_t gsi_base;  // Global System Interrupt base (for IOAPIC)
} irq_controller_desc_t;

// --- Timers ---

typedef enum {
    TIMER_UNKNOWN = 0,
    TIMER_HPET,         // x86_64 HPET
    TIMER_LAPIC,        // x86_64 Local APIC Timer
    TIMER_ARM_GENERIC,  // ARM Generic Timer
    TIMER_RISCV_SBI,    // RISC-V SBI Timer
} timer_type_t;

typedef struct {
    timer_type_t type;
    uint64_t base;
    uint64_t size;
    uint32_t frequency; // 0 if dynamically measured
    uint32_t irq;       // Optional IRQ mapping
} timer_desc_t;

// --- PCI Host Bridges ---

typedef struct {
    uint64_t ecam_base; // Base address of Enhanced Configuration Access Mechanism
    uint64_t ecam_size;
    uint16_t segment;   // PCI Segment Group Number
    uint8_t bus_start;
    uint8_t bus_end;
} pci_host_desc_t;

// --- IOMMUs ---

typedef enum {
    IOMMU_UNKNOWN = 0,
    IOMMU_VTD,          // Intel VT-d
    IOMMU_AMD,          // AMD IOMMU
    IOMMU_SMMU_V2,      // ARM SMMUv2
    IOMMU_SMMU_V3,      // ARM SMMUv3
    IOMMU_IOPMP,        // RISC-V IOPMP
} iommu_type_t;

typedef struct {
    iommu_type_t type;
    uint64_t base;
    uint64_t size;
    uint16_t segment;   // Associated PCI segment
    uint32_t flags;     // Type-specific flags (e.g., Interrupt Remapping supported)
} iommu_desc_t;

// --- Performance Monitoring Units (PMUs) ---

typedef enum {
    PMU_UNKNOWN = 0,
    PMU_ARCH_X86,       // x86_64 Architectural PMU
    PMU_ARMV8,          // ARMv8 PMU
    PMU_RISCV_SBI,      // RISC-V SBI PMU Extension
} pmu_type_t;

typedef struct {
    pmu_type_t type;
    uint32_t num_counters;
    uint32_t irq;       // Overflow interrupt (if supported/routed)
} pmu_desc_t;

// --- Global System Discovery State ---

typedef struct {
    system_topology_t topology;

    uint32_t irq_ctrl_count;
    irq_controller_desc_t irq_ctrls[BHARAT_MAX_IRQ_CONTROLLERS];

    uint32_t timer_count;
    timer_desc_t timers[BHARAT_MAX_TIMERS];

    uint32_t pci_host_count;
    pci_host_desc_t pci_hosts[BHARAT_MAX_PCI_HOSTS];

    uint32_t iommu_count;
    iommu_desc_t iommus[BHARAT_MAX_IOMMUS];

    uint32_t pmu_count;
    pmu_desc_t pmus[BHARAT_MAX_PMUS];
} system_discovery_t;

// Retrieve the global discovery structure
system_discovery_t* hal_get_system_discovery(void);

#endif // BHARAT_HAL_DISCOVERY_H