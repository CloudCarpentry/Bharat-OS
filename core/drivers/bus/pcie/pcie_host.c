#include "pcie_host.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "device/pci.h"
#include "hal/hal_discovery.h"
#include "mm/physmap.h"

static pcie_host_config_t g_pcie_host_cfg;
static int g_pcie_host_initialized = 0;

#define MAX_PCI_DEVICES 32
static pci_device_t g_pci_device_pool[MAX_PCI_DEVICES];
static uint32_t g_pci_device_count = 0;
static pci_device_t* g_pci_device_list_head = NULL;

static volatile void* pci_ecam_addr(uint8_t bus, uint8_t slot, uint8_t func, uint16_t offset) {
    system_discovery_t *disc = hal_get_system_discovery();
    if (!disc) return NULL;

    for (uint32_t i = 0; i < disc->pci_host_count; i++) {
        pci_host_desc_t *host = &disc->pci_hosts[i];
        if (bus >= host->bus_start && bus <= host->bus_end) {
            uint64_t phys_addr = host->ecam_base
                + (((uint64_t)(bus - host->bus_start)) << 20)
                + (((uint64_t)slot) << 15)
                + (((uint64_t)func) << 12)
                + offset;
            return physmap_phys_to_virt(phys_addr);
        }
    }
    return NULL;
}

static uint32_t pci_bdf_read32(uint8_t bus, uint8_t slot, uint8_t func, uint16_t offset) {
    if ((offset & 3) != 0 || offset >= 4096) return 0xFFFFFFFFU;
    volatile uint32_t *addr = (volatile uint32_t *)pci_ecam_addr(bus, slot, func, offset);
    if (!addr) return 0xFFFFFFFFU;
    return *addr;
}

static uint16_t pci_bdf_read16(uint8_t bus, uint8_t slot, uint8_t func, uint16_t offset) {
    if ((offset & 1) != 0 || offset >= 4096) return 0xFFFFU;
    volatile uint16_t *addr = (volatile uint16_t *)pci_ecam_addr(bus, slot, func, offset);
    if (!addr) return 0xFFFFU;
    return *addr;
}

static uint8_t pci_bdf_read8(uint8_t bus, uint8_t slot, uint8_t func, uint16_t offset) {
    if (offset >= 4096) return 0xFFU;
    volatile uint8_t *addr = (volatile uint8_t *)pci_ecam_addr(bus, slot, func, offset);
    if (!addr) return 0xFFU;
    return *addr;
}

static void pci_bdf_write32(uint8_t bus, uint8_t slot, uint8_t func, uint16_t offset, uint32_t val) {
    if ((offset & 3) != 0 || offset >= 4096) return;
    volatile uint32_t *addr = (volatile uint32_t *)pci_ecam_addr(bus, slot, func, offset);
    if (addr) {
        *addr = val;
    }
}

uint32_t pci_config_read32(pci_device_t* dev, uint16_t offset) {
    if (!dev) return 0xFFFFFFFFU;
    return pci_bdf_read32((uint8_t)dev->bus, dev->slot, dev->func, offset);
}

uint16_t pci_config_read16(pci_device_t* dev, uint16_t offset) {
    if (!dev) return 0xFFFFU;
    return pci_bdf_read16((uint8_t)dev->bus, dev->slot, dev->func, offset);
}

uint8_t pci_config_read8(pci_device_t* dev, uint16_t offset) {
    if (!dev) return 0xFFU;
    return pci_bdf_read8((uint8_t)dev->bus, dev->slot, dev->func, offset);
}

void pci_config_write32(pci_device_t* dev, uint16_t offset, uint32_t val) {
    if (!dev) return;
    volatile uint32_t *addr = (volatile uint32_t *)pci_ecam_addr((uint8_t)dev->bus, dev->slot, dev->func, offset);
    if (addr) {
        *addr = val;
    }
}

void pci_config_write16(pci_device_t* dev, uint16_t offset, uint16_t val) {
    if (!dev) return;
    volatile uint16_t *addr = (volatile uint16_t *)pci_ecam_addr((uint8_t)dev->bus, dev->slot, dev->func, offset);
    if (addr) {
        *addr = val;
    }
}

void pci_config_write8(pci_device_t* dev, uint16_t offset, uint8_t val) {
    if (!dev) return;
    volatile uint8_t *addr = (volatile uint8_t *)pci_ecam_addr((uint8_t)dev->bus, dev->slot, dev->func, offset);
    if (addr) {
        *addr = val;
    }
}

int pci_enumerate(void) {
    system_discovery_t *disc = hal_get_system_discovery();
    if (!disc) return -1;

    g_pci_device_count = 0;
    g_pci_device_list_head = NULL;

    for (uint32_t i = 0; i < disc->pci_host_count; i++) {
        pci_host_desc_t *host = &disc->pci_hosts[i];
        for (uint32_t bus = host->bus_start; bus <= host->bus_end; bus++) {
            for (uint8_t slot = 0; slot < 32; slot++) {
                // Check vendor ID for func = 0
                uint16_t vendor_id = pci_bdf_read16((uint8_t)bus, slot, 0, 0);
                if (vendor_id == 0xFFFFU || vendor_id == 0) continue;

                uint8_t header_type = pci_bdf_read8((uint8_t)bus, slot, 0, 0x0E);
                bool is_multi = (header_type & 0x80U) != 0;

                for (uint8_t func = 0; func < 8; func++) {
                    if (func > 0 && !is_multi) break;

                    uint16_t v_id = pci_bdf_read16((uint8_t)bus, slot, func, 0);
                    if (v_id == 0xFFFFU || v_id == 0) continue;

                    if (g_pci_device_count >= MAX_PCI_DEVICES) break;

                    pci_device_t *dev = &g_pci_device_pool[g_pci_device_count++];
                    dev->segment = (uint16_t)host->segment;
                    dev->bus = (uint8_t)bus;
                    dev->slot = slot;
                    dev->func = func;
                    dev->vendor_id = v_id;
                    dev->device_id = pci_bdf_read16((uint8_t)bus, slot, func, 2);

                    uint32_t class_rev = pci_bdf_read32((uint8_t)bus, slot, func, 8);
                    dev->class_id = (uint8_t)(class_rev >> 24);
                    dev->subclass_id = (uint8_t)(class_rev >> 16);
                    dev->prog_if = (uint8_t)(class_rev >> 8);
                    dev->rev_id = (uint8_t)class_rev;

                    // Read BARs
                    for (int b = 0; b < 6; b++) {
                        uint16_t bar_offset = (uint16_t)(0x10 + b * 4);
                        uint32_t orig = pci_bdf_read32((uint8_t)bus, slot, func, bar_offset);
                        pci_bdf_write32((uint8_t)bus, slot, func, bar_offset, 0xFFFFFFFFU);
                        uint32_t mask = pci_bdf_read32((uint8_t)bus, slot, func, bar_offset);
                        pci_bdf_write32((uint8_t)bus, slot, func, bar_offset, orig);

                        dev->bar[b] = orig;
                        if (mask != 0 && mask != 0xFFFFFFFFU) {
                            uint32_t lsb;
                            if ((orig & 1) == 0) {
                                lsb = mask & 0xFFFFFFF0U;
                                dev->bar_flags[b] = orig & 0xFU;
                            } else {
                                lsb = mask & 0xFFFFFFFCU;
                                dev->bar_flags[b] = orig & 0x3U;
                            }
                            dev->bar_size[b] = ~lsb + 1;
                        } else {
                            dev->bar_size[b] = 0;
                            dev->bar_flags[b] = 0;
                        }
                    }

                    // Parse capabilities
                    uint8_t cap_ptr = pci_bdf_read8((uint8_t)bus, slot, func, 0x34);
                    dev->msi_cap_offset = 0;
                    dev->msix_cap_offset = 0;
                    dev->pcie_cap_offset = 0;
                    while (cap_ptr != 0 && cap_ptr < 0xFC) {
                        uint8_t cap_id = pci_bdf_read8((uint8_t)bus, slot, func, cap_ptr);
                        if (cap_id == 0x05) {
                            dev->msi_cap_offset = cap_ptr;
                        } else if (cap_id == 0x11) {
                            dev->msix_cap_offset = cap_ptr;
                        } else if (cap_id == 0x10) {
                            dev->pcie_cap_offset = cap_ptr;
                        }
                        cap_ptr = pci_bdf_read8((uint8_t)bus, slot, func, (uint16_t)(cap_ptr + 1));
                    }

                    dev->is_pcie = (dev->pcie_cap_offset != 0);
                    dev->next = g_pci_device_list_head;
                    g_pci_device_list_head = dev;
                }
            }
        }
    }

    return (int)g_pci_device_count;
}

int pci_enable_device(pci_device_t* dev) {
    if (!dev) return -1;
    uint16_t cmd = pci_config_read16(dev, 0x04);
    cmd |= (1U << 1) | (1U << 2); // Memory Space + Bus Master
    pci_config_write16(dev, 0x04, cmd);
    return 0;
}

int pci_enable_msi(pci_device_t* dev, int vectors, void* desc_array) {
    (void)dev;
    (void)vectors;
    (void)desc_array;
    return 0; // Simulated success
}

void pci_disable_msi(pci_device_t* dev) {
    (void)dev;
}

pci_device_t* pci_get_device_list(void) {
    return g_pci_device_list_head;
}

int pcie_host_init(const pcie_host_config_t* cfg) {
    if (cfg == NULL) {
        return -1;
    }
    if ((cfg->bus_end < cfg->bus_start) || (cfg->ecam_base == 0u)) {
        return -2;
    }
    g_pcie_host_cfg = *cfg;
    g_pcie_host_initialized = 1;
    return 0;
}

int pcie_host_enumerate(uint8_t* discovered_devices) {
    if (discovered_devices == NULL) {
        return -1;
    }
    int count = pci_enumerate();
    if (count < 0) {
        *discovered_devices = 0;
        return count;
    }
    *discovered_devices = (uint8_t)count;
    return 0;
}
