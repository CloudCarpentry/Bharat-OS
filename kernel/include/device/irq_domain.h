#ifndef BHARAT_IRQ_DOMAIN_H
#define BHARAT_IRQ_DOMAIN_H

#include <stdint.h>
#include <stdbool.h>
#include "hal/hal_discovery.h"

// --- IRQ Domains ---

typedef struct irq_domain irq_domain_t;

struct irq_domain {
    const char* name;
    uint32_t irq_base;
    uint32_t irq_count;

    // Domain operations
    int (*map)(irq_domain_t* domain, uint32_t virq, uint32_t hwirq);
    void (*unmap)(irq_domain_t* domain, uint32_t virq);

    // Domain-specific data
    void* host_data;
    irq_domain_t* parent;
};

// Create a new IRQ domain
irq_domain_t* irq_domain_create(const char* name, uint32_t irq_base, uint32_t irq_count, void* host_data);

// Map a hardware IRQ to a virtual IRQ within the domain
int irq_domain_map(irq_domain_t* domain, uint32_t virq, uint32_t hwirq);

// --- MSI Domains ---

typedef struct msi_msg {
    uint64_t address;
    uint32_t data;
} msi_msg_t;

typedef struct msi_desc {
    uint32_t irq;
    msi_msg_t msg;
    void* device; // E.g., pointer to pci_device_t
    bool is_msix;
    uint16_t msix_entry;
} msi_desc_t;

typedef struct msi_domain msi_domain_t;

struct msi_domain {
    const char* name;
    irq_domain_t* base_domain;

    // MSI domain operations
    int (*alloc_msi)(msi_domain_t* domain, void* device, int count, msi_desc_t* desc_array);
    void (*free_msi)(msi_domain_t* domain, msi_desc_t* desc_array, int count);
    int (*write_msg)(msi_domain_t* domain, msi_desc_t* desc);

    void* host_data;
};

// Register a new MSI domain (typically by an architecture backend like ITS or VT-d)
int msi_domain_register(msi_domain_t* domain);

// Get the system default MSI domain
msi_domain_t* msi_domain_get_default(void);

// Allocate MSIs for a device
int msi_domain_alloc_irqs(msi_domain_t* domain, void* device, int count, msi_desc_t* desc_array);

// Free allocated MSIs
void msi_domain_free_irqs(msi_domain_t* domain, msi_desc_t* desc_array, int count);

#endif // BHARAT_IRQ_DOMAIN_H