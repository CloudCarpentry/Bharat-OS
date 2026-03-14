#include "hal/hal_topology.h"
#include "hal/hal_boot.h"

#include <stddef.h>
#include <stdint.h>

// Minimal ACPI parser to find CPUs (MADT) and memory (SRAT - skipped for minimal phase 1)

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

static int str_eq_len(const char* a, const char* b, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (a[i] != b[i]) return 0;
    }
    return 1;
}

int hal_topology_init(void) {
    bharat_boot_info_t* boot_info = hal_boot_get_info();
    if (!boot_info) return -1;

    struct rsdp_descriptor* rsdp = (struct rsdp_descriptor*)boot_info->acpi_rsdp;
    if (!rsdp) return -1; // Fallback to UMA

    // Find MADT from RSDT
    uint32_t* rsdt = (uint32_t*)((uintptr_t)rsdp->rsdt_address);
    struct acpi_sdt_header* rsdt_header = (struct acpi_sdt_header*)rsdt;

    size_t entries = (rsdt_header->length - sizeof(struct acpi_sdt_header)) / 4;
    struct madt_header* madt = NULL;

    for (size_t i = 0; i < entries; i++) {
        struct acpi_sdt_header* h = (struct acpi_sdt_header*)((uintptr_t)rsdt[i + (sizeof(struct acpi_sdt_header)/4)]);
        if (str_eq_len(h->signature, "APIC", 4)) {
            madt = (struct madt_header*)h;
            break;
        }
    }

    if (madt) {
        boot_info->cpu_count = 0;
        uint8_t* ptr = (uint8_t*)madt + sizeof(struct madt_header);
        uint8_t* end = (uint8_t*)madt + madt->header.length;

        while (ptr < end) {
            struct madt_entry_header* h = (struct madt_entry_header*)ptr;
            if (h->type == 0) { // Local APIC
                struct madt_local_apic* lapic = (struct madt_local_apic*)ptr;
                if ((lapic->flags & 1) && boot_info->cpu_count < BHARAT_MAX_CPUS) { // Enabled
                    boot_info->cpus[boot_info->cpu_count].cpu_id = boot_info->cpu_count;
                    boot_info->cpus[boot_info->cpu_count].apic_id = lapic->apic_id;
                    boot_info->cpus[boot_info->cpu_count].is_bsp = (boot_info->cpu_count == 0);
                    boot_info->cpu_count++;
                }
            }
            ptr += h->length;
        }
    }

    return 0; // Returning 0 indicates successful parsing or UMA fallback
}
