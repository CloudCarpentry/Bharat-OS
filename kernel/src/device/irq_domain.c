#include "device/irq_domain.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Simple placeholder implementation for IRQ/MSI domains

static msi_domain_t* g_default_msi_domain = NULL;

irq_domain_t* irq_domain_create(const char* name, uint32_t irq_base, uint32_t irq_count, void* host_data) {
    // A real implementation would allocate and track the domain
    // For now, return a dummy pointer
    return (irq_domain_t*)(uintptr_t)0x1234;
}

int irq_domain_map(irq_domain_t* domain, uint32_t virq, uint32_t hwirq) {
    if (!domain) return -1;
    // Map implementation
    return 0;
}

int msi_domain_register(msi_domain_t* domain) {
    if (!domain) return -1;
    if (!g_default_msi_domain) {
        g_default_msi_domain = domain;
    }
    return 0;
}

msi_domain_t* msi_domain_get_default(void) {
    return g_default_msi_domain;
}

int msi_domain_alloc_irqs(msi_domain_t* domain, void* device, int count, msi_desc_t* desc_array) {
    if (!domain || !desc_array || count <= 0) return -1;
    if (domain->alloc_msi) {
        return domain->alloc_msi(domain, device, count, desc_array);
    }
    return -1;
}

void msi_domain_free_irqs(msi_domain_t* domain, msi_desc_t* desc_array, int count) {
    if (!domain || !desc_array || count <= 0) return;
    if (domain->free_msi) {
        domain->free_msi(domain, desc_array, count);
    }
}
