#include "hal/hal_topology.h"
#include "hal/hal_boot.h"
#include "hal/hal_discovery.h"

#include <stddef.h>
#include <stdint.h>

// Minimal ACPI parser to find CPUs (MADT) and memory/devices via SRAT, HPET, MCFG, DMAR

struct rsdp_descriptor {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
} __attribute__((packed));

struct acpi_sdt_header {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed));

struct madt_header {
    struct acpi_sdt_header header;
    uint32_t local_apic_address;
    uint32_t flags;
} __attribute__((packed));

struct madt_entry_header {
    uint8_t type;
    uint8_t length;
} __attribute__((packed));

struct madt_local_apic {
    struct madt_entry_header header;
    uint8_t acpi_processor_id;
    uint8_t apic_id;
    uint32_t flags;
} __attribute__((packed));

struct madt_io_apic {
    struct madt_entry_header header;
    uint8_t io_apic_id;
    uint8_t reserved;
    uint32_t io_apic_address;
    uint32_t global_system_interrupt_base;
} __attribute__((packed));

struct hpet_header {
    struct acpi_sdt_header header;
    uint8_t hw_rev_id;
    uint8_t comparator_count:5;
    uint8_t counter_size:1;
    uint8_t reserved:1;
    uint8_t legacy_replacement:1;
    uint16_t pci_vendor_id;
    uint8_t address_space_id;    // 0 - System Memory, 1 - System I/O
    uint8_t register_bit_width;
    uint8_t register_bit_offset;
    uint8_t reserved2;
    uint64_t address;
    uint8_t hpet_number;
    uint16_t minimum_tick;
    uint8_t page_protection;
} __attribute__((packed));

struct mcfg_header {
    struct acpi_sdt_header header;
    uint64_t reserved;
} __attribute__((packed));

struct mcfg_allocation {
    uint64_t base_address;
    uint16_t pci_segment_group_number;
    uint8_t start_bus_number;
    uint8_t end_bus_number;
    uint32_t reserved;
} __attribute__((packed));

struct dmar_header {
    struct acpi_sdt_header header;
    uint8_t host_address_width;
    uint8_t flags;
    uint8_t reserved[10];
} __attribute__((packed));

static int str_eq_len(const char* a, const char* b, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (a[i] != b[i]) return 0;
    }
    return 1;
}

static void parse_madt(struct madt_header* madt, bharat_boot_info_t* boot_info) {
    system_discovery_t* disc = hal_get_system_discovery();

    boot_info->cpu_count = 0;
    disc->topology.cpu_count = 0;
    uint8_t* ptr = (uint8_t*)madt + sizeof(struct madt_header);
    uint8_t* end = (uint8_t*)madt + madt->header.length;

    // LAPIC is the controller for x86 CPUs
    disc->irq_ctrls[disc->irq_ctrl_count].type = IRQ_CTRL_APIC;
    disc->irq_ctrls[disc->irq_ctrl_count].base = madt->local_apic_address;
    disc->irq_ctrls[disc->irq_ctrl_count].size = 0x1000;
    disc->irq_ctrl_count++;

    while (ptr < end) {
        struct madt_entry_header* h = (struct madt_entry_header*)ptr;
        if (h->type == 0) { // Local APIC
            struct madt_local_apic* lapic = (struct madt_local_apic*)ptr;
            if ((lapic->flags & 1) && boot_info->cpu_count < BHARAT_MAX_CPUS) { // Enabled
                // Legacy boot_info
                boot_info->cpus[boot_info->cpu_count].cpu_id = boot_info->cpu_count;
                boot_info->cpus[boot_info->cpu_count].apic_id = lapic->apic_id;
                boot_info->cpus[boot_info->cpu_count].is_bsp = (boot_info->cpu_count == 0);
                boot_info->cpu_count++;

                // New discovery struct
                uint32_t cid = disc->topology.cpu_count;
                disc->topology.cpus[cid].cpu_id = cid;
                disc->topology.cpus[cid].hw_id = lapic->apic_id;
                disc->topology.cpus[cid].node_id = 0; // Update via SRAT later
                disc->topology.cpus[cid].is_bsp = (cid == 0);
                disc->topology.cpu_count++;
            }
        } else if (h->type == 1) { // IO APIC
            struct madt_io_apic* ioapic = (struct madt_io_apic*)ptr;
            if (disc->irq_ctrl_count < BHARAT_MAX_IRQ_CONTROLLERS) {
                disc->irq_ctrls[disc->irq_ctrl_count].type = IRQ_CTRL_IOAPIC;
                disc->irq_ctrls[disc->irq_ctrl_count].base = ioapic->io_apic_address;
                disc->irq_ctrls[disc->irq_ctrl_count].size = 0x1000; // standard MMIO size
                disc->irq_ctrls[disc->irq_ctrl_count].id = ioapic->io_apic_id;
                disc->irq_ctrls[disc->irq_ctrl_count].gsi_base = ioapic->global_system_interrupt_base;
                disc->irq_ctrl_count++;
            }
        }
        ptr += h->length;
    }
}

static void parse_hpet(struct hpet_header* hpet) {
    system_discovery_t* disc = hal_get_system_discovery();
    if (disc->timer_count < BHARAT_MAX_TIMERS) {
        disc->timers[disc->timer_count].type = TIMER_HPET;
        disc->timers[disc->timer_count].base = hpet->address;
        disc->timers[disc->timer_count].size = 0x400; // typical HPET size
        disc->timer_count++;
    }
}

static void parse_mcfg(struct mcfg_header* mcfg) {
    system_discovery_t* disc = hal_get_system_discovery();
    uint8_t* ptr = (uint8_t*)mcfg + sizeof(struct mcfg_header);
    uint8_t* end = (uint8_t*)mcfg + mcfg->header.length;

    while (ptr < end && disc->pci_host_count < BHARAT_MAX_PCI_HOSTS) {
        struct mcfg_allocation* alloc = (struct mcfg_allocation*)ptr;
        disc->pci_hosts[disc->pci_host_count].ecam_base = alloc->base_address;
        disc->pci_hosts[disc->pci_host_count].ecam_size = ((alloc->end_bus_number - alloc->start_bus_number) + 1) << 20; // 1MB per bus
        disc->pci_hosts[disc->pci_host_count].segment = alloc->pci_segment_group_number;
        disc->pci_hosts[disc->pci_host_count].bus_start = alloc->start_bus_number;
        disc->pci_hosts[disc->pci_host_count].bus_end = alloc->end_bus_number;
        disc->pci_host_count++;
        ptr += sizeof(struct mcfg_allocation);
    }
}

static void parse_dmar(struct dmar_header* dmar) {
    system_discovery_t* disc = hal_get_system_discovery();
    if (disc->iommu_count < BHARAT_MAX_IOMMUS) {
        disc->iommus[disc->iommu_count].type = IOMMU_VTD;
        // The DMAR table contains sub-tables (DRHD, RMRR, etc.)
        // A minimal parse would find the DRHD structures to get base addresses.
        // As a placeholder, we just note VT-d presence. Full parsing is done in vtd_iommu.c
        disc->iommus[disc->iommu_count].base = 0; // To be populated by DRHD parser
        disc->iommus[disc->iommu_count].flags = dmar->flags; // Check INTR_REMAP bit (bit 0)
        disc->iommu_count++;
    }
}

int hal_topology_init(void) {
    bharat_boot_info_t* boot_info = hal_boot_get_info();
    if (!boot_info) return -1;

    struct rsdp_descriptor* rsdp = (struct rsdp_descriptor*)boot_info->acpi_rsdp;
    if (!rsdp) return -1; // Fallback to UMA

    // Parse tables
    uint32_t* rsdt = (uint32_t*)((uintptr_t)rsdp->rsdt_address);
    struct acpi_sdt_header* rsdt_header = (struct acpi_sdt_header*)rsdt;

    size_t entries = (rsdt_header->length - sizeof(struct acpi_sdt_header)) / 4;

    for (size_t i = 0; i < entries; i++) {
        struct acpi_sdt_header* h = (struct acpi_sdt_header*)((uintptr_t)rsdt[i + (sizeof(struct acpi_sdt_header)/4)]);
        if (str_eq_len(h->signature, "APIC", 4)) { // MADT
            parse_madt((struct madt_header*)h, boot_info);
        } else if (str_eq_len(h->signature, "HPET", 4)) {
            parse_hpet((struct hpet_header*)h);
        } else if (str_eq_len(h->signature, "MCFG", 4)) {
            parse_mcfg((struct mcfg_header*)h);
        } else if (str_eq_len(h->signature, "DMAR", 4)) {
            parse_dmar((struct dmar_header*)h);
        }
        // TODO: SRAT and SLIT for NUMA memory/distance
    }

    return 0;
}
