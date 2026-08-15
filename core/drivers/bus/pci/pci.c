#include "device.h"
#include <stdint.h>
#include <stddef.h>

#if defined(__x86_64__)

#ifndef TESTING
static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t val;
    __asm__ volatile("inl %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}
#else
static inline void outl(uint16_t port, uint32_t val) {
    (void)port;
    (void)val;
}

static inline uint32_t inl(uint16_t port) {
    (void)port;
    return 0xFFFFFFFF; // Nothing present by default
}
#endif

static uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;

    // Create configuration address as per PCI spec
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));

    outl(0xCF8, address);
    return inl(0xCFC);
}

static void pci_write_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
    address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    outl(0xCF8, address);
    outl(0xCFC, val);
}

int pci_discover_nic(device_mmio_window_t* rx_window, device_mmio_window_t* tx_window) {
    if (!rx_window || !tx_window) return -1;

    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            uint32_t vendor_device = pci_read_config((uint8_t)bus, slot, 0, 0);
            if (vendor_device == 0xFFFFFFFF) continue; // No device present

            uint32_t class_subclass = pci_read_config((uint8_t)bus, slot, 0, 0x08);
            uint8_t base_class = (class_subclass >> 24) & 0xFF;

            // 0x02 is Network Controller
            if (base_class == 0x02) {
                // Read BAR0 and BAR1
                uint32_t bar0 = pci_read_config((uint8_t)bus, slot, 0, 0x10);
                uint32_t bar1 = pci_read_config((uint8_t)bus, slot, 0, 0x14);

                // Extremely simplified BAR parsing (assuming memory-mapped 32-bit for stub purposes)
                rx_window->phys_base = bar0 & 0xFFFFFFF0;
                tx_window->phys_base = bar1 & 0xFFFFFFF0;

                // Fallbacks if not set
                if (rx_window->phys_base == 0) rx_window->phys_base = 0x40000000U;
                if (tx_window->phys_base == 0) tx_window->phys_base = 0x40010000U;

                rx_window->virt_base = 0x8000000000U;
                tx_window->virt_base = 0x8000010000U;
                rx_window->size_bytes = 0x10000U;
                tx_window->size_bytes = 0x10000U;
                rx_window->irq = 10U;
                tx_window->irq = 10U;
                rx_window->in_use = 1U;
                tx_window->in_use = 1U;

                return 0; // Found NIC
            }
        }
    }
    return -2; // Not found
}

#else

static uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    (void)bus;
    (void)slot;
    (void)func;
    (void)offset;
    return 0xFFFFFFFF;
}

static void pci_write_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val) {
    (void)bus;
    (void)slot;
    (void)func;
    (void)offset;
    (void)val;
}

int pci_discover_nic(device_mmio_window_t* rx_window, device_mmio_window_t* tx_window) {
    (void)rx_window;
    (void)tx_window;
    return -2; // Unsupported architecture stub
}

#endif

// --- MSI Parsing Groundwork ---
#define PCI_CAP_ID_MSI 0x05
#define PCI_CAP_ID_MSIX 0x11

// Mock structure for demonstration and compilation
typedef struct {
    uint8_t bus;
    uint8_t slot;
    uint8_t func;
    uint32_t irq;
} pci_device_t;

int pci_enable_msi(pci_device_t* dev, uint32_t irq, uint64_t msi_address, uint32_t msi_data) {
    if (!dev) return -1;

    // Read Status Register to check for Capabilities List bit
    uint32_t status_cmd = pci_read_config(dev->bus, dev->slot, dev->func, 0x04);
    if (!(status_cmd & 0x00100000)) {
        return -1; // No capabilities list
    }

    // Read Capabilities Pointer
    uint32_t cap_ptr_reg = pci_read_config(dev->bus, dev->slot, dev->func, 0x34);
    uint8_t cap_ptr = cap_ptr_reg & 0xFF;

    while (cap_ptr != 0) {
        uint32_t cap_header = pci_read_config(dev->bus, dev->slot, dev->func, cap_ptr);
        uint8_t cap_id = cap_header & 0xFF;

        if (cap_id == PCI_CAP_ID_MSI) {
            // MSI Capability found!

            // Note: Simplistic 32-bit vs 64-bit message address parsing
            uint16_t msg_ctrl = (cap_header >> 16) & 0xFFFF;
            bool is_64bit = (msg_ctrl & 0x0080) != 0;

            // Write MSI Address
            pci_write_config(dev->bus, dev->slot, dev->func, cap_ptr + 4, (uint32_t)msi_address);

            if (is_64bit) {
                pci_write_config(dev->bus, dev->slot, dev->func, cap_ptr + 8, (uint32_t)(msi_address >> 32));
                pci_write_config(dev->bus, dev->slot, dev->func, cap_ptr + 12, msi_data);
            } else {
                pci_write_config(dev->bus, dev->slot, dev->func, cap_ptr + 8, msi_data);
            }

            // Enable MSI
            msg_ctrl |= 0x0001;
            uint32_t new_cap_header = (cap_header & 0x0000FFFF) | ((uint32_t)msg_ctrl << 16);
            pci_write_config(dev->bus, dev->slot, dev->func, cap_ptr, new_cap_header);

            dev->irq = irq;
            return 0;
        }

        cap_ptr = (cap_header >> 8) & 0xFF;
    }

    return -1; // MSI Capability not found
}
