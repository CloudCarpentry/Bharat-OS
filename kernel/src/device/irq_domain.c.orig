#include "device/irq_domain.h"
#include "lib/base/string.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Phase-1 in-kernel IRQ domain table:
// - Static storage (no dynamic allocation in core IRQ path)
// - Explicit map/unmap/translate tracking
// - Optional parent hierarchy pointer stored in descriptor

static msi_domain_t* g_default_msi_domain = NULL;

typedef struct irq_map_entry {
    irq_domain_t* domain;
    uint32_t virq;
    uint32_t hwirq;
    bool in_use;
} irq_map_entry_t;

#define IRQ_DOMAIN_MAX_DOMAINS 32U
#define IRQ_DOMAIN_MAX_MAPPINGS 512U

static irq_domain_t g_domains[IRQ_DOMAIN_MAX_DOMAINS];
static irq_map_entry_t g_maps[IRQ_DOMAIN_MAX_MAPPINGS];
static uint32_t g_domain_count = 0U;

static bool irq_domain_virq_in_range(const irq_domain_t* domain, uint32_t virq) {
    if (!domain || domain->irq_count == 0U) {
        return false;
    }
    if (virq < domain->irq_base) {
        return false;
    }
    return virq < (domain->irq_base + domain->irq_count);
}

irq_domain_t* irq_domain_create(const char* name, uint32_t irq_base, uint32_t irq_count, void* host_data) {
    if (!name || irq_count == 0U) {
        return NULL;
    }
    if (g_domain_count >= IRQ_DOMAIN_MAX_DOMAINS) {
        return NULL;
    }

    irq_domain_t* domain = &g_domains[g_domain_count++];
    memset(domain, 0, sizeof(*domain));
    domain->name = name;
    domain->irq_base = irq_base;
    domain->irq_count = irq_count;
    domain->host_data = host_data;
    return domain;
}

int irq_domain_map(irq_domain_t* domain, uint32_t virq, uint32_t hwirq) {
    uint32_t i;
    int free_slot = -1;

    if (!domain) return -1;
    if (!irq_domain_virq_in_range(domain, virq)) return -1;

    for (i = 0; i < IRQ_DOMAIN_MAX_MAPPINGS; i++) {
        if (g_maps[i].in_use) {
            // Reject duplicate virtual IRQ and duplicate hwirq in same domain.
            if (g_maps[i].domain == domain && (g_maps[i].virq == virq || g_maps[i].hwirq == hwirq)) {
                return -1;
            }
        } else if (free_slot < 0) {
            free_slot = (int)i;
        }
    }

    if (free_slot < 0) {
        return -1;
    }

    g_maps[free_slot].domain = domain;
    g_maps[free_slot].virq = virq;
    g_maps[free_slot].hwirq = hwirq;
    g_maps[free_slot].in_use = true;

    if (domain->map) {
        return domain->map(domain, virq, hwirq);
    }

    return 0;
}

int irq_domain_unmap(irq_domain_t* domain, uint32_t virq) {
    uint32_t i;
    if (!domain) return -1;

    for (i = 0; i < IRQ_DOMAIN_MAX_MAPPINGS; i++) {
        if (g_maps[i].in_use && g_maps[i].domain == domain && g_maps[i].virq == virq) {
            g_maps[i].in_use = false;
            g_maps[i].domain = NULL;
            if (domain->unmap) {
                domain->unmap(domain, virq);
            }
            return 0;
        }
    }
    return -1;
}

int irq_domain_translate(irq_domain_t* domain, uint32_t hwirq, uint32_t* out_virq) {
    uint32_t i;
    if (!domain || !out_virq) {
        return -1;
    }

    for (i = 0; i < IRQ_DOMAIN_MAX_MAPPINGS; i++) {
        if (g_maps[i].in_use && g_maps[i].domain == domain && g_maps[i].hwirq == hwirq) {
            *out_virq = g_maps[i].virq;
            return 0;
        }
    }
    return -1;
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
