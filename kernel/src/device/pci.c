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

int pci_discover_nic(device_mmio_window_t* rx_window, device_mmio_window_t* tx_window) {
    (void)rx_window;
    (void)tx_window;
    return -2; // Unsupported architecture stub
}

#endif
