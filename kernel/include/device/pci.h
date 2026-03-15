#ifndef BHARAT_DEVICE_PCI_H
#define BHARAT_DEVICE_PCI_H

#include <stdint.h>
#include <stdbool.h>

typedef struct pci_device {
    uint16_t segment;
    uint8_t bus;
    uint8_t slot;
    uint8_t func;

    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_id;
    uint8_t subclass_id;
    uint8_t prog_if;
    uint8_t rev_id;

    uint32_t bar[6];
    uint32_t bar_size[6];
    uint32_t bar_flags[6];

    bool is_pcie;
    uint16_t pcie_cap_offset;
    uint16_t msi_cap_offset;
    uint16_t msix_cap_offset;

    // Linked list or array next ptr
    struct pci_device* next;
} pci_device_t;

// PCI configuration space access functions
uint32_t pci_config_read32(pci_device_t* dev, uint16_t offset);
uint16_t pci_config_read16(pci_device_t* dev, uint16_t offset);
uint8_t  pci_config_read8(pci_device_t* dev, uint16_t offset);

void pci_config_write32(pci_device_t* dev, uint16_t offset, uint32_t val);
void pci_config_write16(pci_device_t* dev, uint16_t offset, uint16_t val);
void pci_config_write8(pci_device_t* dev, uint16_t offset, uint8_t val);

// Enumerate buses starting from ECAM descriptors
int pci_enumerate(void);

// Enable bus mastering for DMA and MMIO space
int pci_enable_device(pci_device_t* dev);

// Enable MSI/MSI-X
int pci_enable_msi(pci_device_t* dev, int vectors, void* desc_array);
void pci_disable_msi(pci_device_t* dev);

#endif // BHARAT_DEVICE_PCI_H